
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef  _BOOT_FRAME_H_
#define _BOOT_FRAME_H_

#include "frame_def.h"
#include "base_frame.h"
#include "object_pool_helper.h"



static constexpr u32 kPageOrder = 20; //1m  
static constexpr u32 kHeapSpaceOrder = 8; // 8:256,  10:1024  

#define SPACE_ALIGN(bytes) zmalloc_align_value(bytes, 16)


enum  ShmSpace : u32
{
    kMainFrame = 0,
    kPool,
    kBuddy,
    kMalloc,
    kHeap,
};





template <class Frame>  //derive from BaseFrame  
class FrameDelegate
{
public:
    static_assert(std::is_base_of<BaseFrame, Frame>::value, "");
public:
    static Frame& Instance()
    {
        return *space<Frame, ShmSpace::kMainFrame>();
    }

    static inline char*& ShmInstance()
    {
        static char* g_instance_ptr = nullptr;
        return g_instance_ptr;
    }

    static inline zshm_space_entry& SpaceEntry()
    {
        return *(zshm_space_entry*)(ShmInstance());
    }
    template <class T, u32 ID>
    static inline T* space()
    {
        return (T*)(ShmInstance() + SpaceEntry().spaces_[ID].offset_);
    }
private:
    template<class T, class ... Args>
    static inline void RebuildVPTR(void* addr, Args ... args)
    {
        zshm_ptr<T>(addr).fix_vptr();
    }
    template<class T, class ... Args>
    static inline void BuildObject(void* addr, Args ... args)
    {
        new (addr) T(args...);
    }
    template<class T>
    static inline void DestroyObject(T* addr)
    {
        addr->~T();
    }

    static inline void* AllocLarge(u64 bytes)
    {
        bytes += zbuddy_shift_size(kPageOrder) - 1;
        u32 pages = (u32)(bytes >> kPageOrder);
        u64 page_index = space<zbuddy, ShmSpace::kBuddy>()->alloc_page(pages);
        void* addr = space<char, ShmSpace::kHeap>() + (page_index << kPageOrder);
        return addr;
    }
    static inline u64 FreeLarge(void* addr, u64 bytes)
    {
        u64 offset = (char*)addr - space<char, ShmSpace::kHeap>();
        u32 page_index = (u32)(offset >> kPageOrder);
        u32 pages = space<zbuddy, ShmSpace::kBuddy>()->free_page(page_index);
        u64 free_bytes = pages << kPageOrder;
        if (free_bytes < bytes)
        {
            LogError() << "";
        }
        return free_bytes;
    }
public:
    static inline s32 InitSpaceFromConfig(zshm_space_entry& params, bool isUseHeap);
    static inline s32 BuildShm(bool isUseHeap);
    static inline s32 ResumeShm(bool isUseHeap);
    static inline s32 HoldShm(bool isUseHeap);
    static inline s32 DestroyShm(bool isUseHeap, bool self, bool force);
private:
    static ObjectPoolHelper helper_;
};

template <class Frame>
ObjectPoolHelper FrameDelegate<Frame>::helper_;

template <class Frame>  
s32 FrameDelegate<Frame>::InitSpaceFromConfig(zshm_space_entry& params, bool isUseHeap)
{
    memset(&params, 0, sizeof(params));
    params.shm_key_ = 198709;
    params.use_heap_ = isUseHeap;
#ifdef WIN32

#else
    params.use_fixed_address_ = true;
    params.space_addr_ = 0x00006AAAAAAAAAAAULL;
    params.space_addr_ = 0x0000700000000000ULL;
#endif // WIN32

    s32 ret = helper_.AddPool(23, 10, 100, "test");
    if (ret != 0)
    {
        return ret;
    }


    params.spaces_[ShmSpace::kMainFrame].size_ = SPACE_ALIGN(sizeof(Frame));
    params.spaces_[ShmSpace::kPool].size_ = kPoolHeadSize + helper_.TotalSpaceSize();
    params.spaces_[ShmSpace::kBuddy].size_ = SPACE_ALIGN(zbuddy::zbuddy_size(kHeapSpaceOrder));
    params.spaces_[ShmSpace::kMalloc].size_ = SPACE_ALIGN(zmalloc::zmalloc_size());
    params.spaces_[ShmSpace::kHeap].size_ = SPACE_ALIGN(zbuddy_shift_size(kHeapSpaceOrder + kPageOrder));

    params.whole_space_.size_ += SPACE_ALIGN(sizeof(params));
    for (u32 i = 0; i < ZSHM_MAX_SPACES; i++)
    {
        params.spaces_[i].offset_ = params.whole_space_.size_;
        params.whole_space_.size_ += params.spaces_[i].size_;
    }
    return 0;
}


template <class Frame>
s32 FrameDelegate<Frame>::BuildShm(bool isUseHeap)
{
    zshm_space_entry params;
    InitSpaceFromConfig(params, isUseHeap);


    zshm_boot booter;
    zshm_space_entry* shm_space = nullptr;
    s32 ret = booter.build_frame(params, shm_space);
    if (ret != 0 || shm_space == nullptr)
    {
        LogError() << "build_frame error. shm_space:" << (void*)shm_space << ", ret:" << ret;
        return ret;
    }
    ShmInstance() = (char*)shm_space;

    SpaceEntry().space_addr_ = (u64)shm_space;

    BuildObject<Frame>(space<Frame, ShmSpace::kMainFrame>());

    ObjectPoolHead* pool_head = space<ObjectPoolHead, ShmSpace::kPool>();
    memcpy(pool_head, helper_.head(), kPoolHeadSize);
    pool_head->symbols_.attach(pool_head->object_names_, kLimitObjectNameBuffSize, kLimitObjectNameBuffSize); //rebuild symbols   
    char* pool_addr = (char*)pool_head + kPoolHeadSize;

    for (s32 i = 0; i <= pool_head->pool_max_used_id_; i++)
    {
        if (pool_head->pools_[i].space_size_ == 0)
        {
            continue;
        }
        //rebuild bitesets in real space   
        pool_head->bitsets_[i].attach((u64*)pool_addr, pool_head->bitsets_[i].array_size(), true);
        pool_addr += pool_head->bitsets_[i].array_size() * zbitset::kByteSize;
        //rebuild pool in real space  
        zmem_pool& pool = pool_head->pools_[i];
        s32 ret = pool.init(pool.user_size_, pool.user_name_, pool.chunk_count_, pool_addr, pool.space_size_);
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
        pool_addr += pool.space_size_;
        LogDebug() << "init object[" << pool_head->symbols_.at(pool.user_name_) << "] size:" << pool.user_size_ << ", count:" << pool.chunk_count_;
    }


    zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
    memset(buddy_ptr, 0, params.spaces_[ShmSpace::kBuddy].size_);
    buddy_ptr->set_global(buddy_ptr);
    zbuddy::build_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kHeapSpaceOrder, &ret);
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }

    zmalloc* malloc_ptr = space<zmalloc, ShmSpace::kMalloc>();
    memset(malloc_ptr, 0, zmalloc::zmalloc_size());
    malloc_ptr->set_global(malloc_ptr);
    malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
    malloc_ptr->check_health();


    ret = space<Frame, ShmSpace::kMainFrame>()->Start();
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }

    return 0;
}



template <class Frame>
s32 FrameDelegate<Frame>::ResumeShm(bool isUseHeap)
{
    zshm_space_entry params;
    InitSpaceFromConfig(params, isUseHeap);
    zshm_boot booter;
    zshm_space_entry* shm_space = nullptr;
    s32 ret = booter.resume_frame(params, shm_space);
    if (ret != 0 || shm_space == nullptr)
    {
        LogError() << "booter.resume_frame error. shm_space:" << (void*)shm_space << ", ret:" << ret;
        return ret;
    }
    ShmInstance() = (char*)shm_space;

    SpaceEntry().space_addr_ = (u64)shm_space;
    Frame* m = space<Frame, ShmSpace::kMainFrame>();
    RebuildVPTR<Frame>(m);


    ObjectPoolHead* pool_head = space<ObjectPoolHead, ShmSpace::kPool>();
    if (helper_.CheckVersion(pool_head) != 0)
    {
        LogError() << "";
        return ret;
    }

    for (s32 i = 0; i <= pool_head->pool_max_used_id_; i++)
    {
        //resume object vptr
        zmem_pool& pool = pool_head->pools_[i];
        LogDebug() << "resume object[" << pool_head->symbols_.at(pool.user_name_) << "] size:" << pool.user_size_ << ", count:" << pool.chunk_count_;
    }

    zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
    buddy_ptr->set_global(buddy_ptr);
    zbuddy::rebuild_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kHeapSpaceOrder, &ret);
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }

    zmalloc* malloc_ptr = space<zmalloc, ShmSpace::kMalloc>();
    malloc_ptr->set_global(malloc_ptr);
    malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
    malloc_ptr->check_health();

    ret = space<Frame, ShmSpace::kMainFrame>()->Resume();
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }
    return 0;
}
template <class Frame>
s32 FrameDelegate<Frame>::HoldShm(bool isUseHeap)
{
    if (isUseHeap)
    {
        LogError() << "";
        return -1;
    }
    zshm_space_entry params;
    InitSpaceFromConfig(params, isUseHeap);
    zshm_boot booter;
    zshm_space_entry* shm_space = nullptr;
    s32 ret = booter.resume_frame(params, shm_space);
    if (ret != 0 || shm_space == nullptr)
    {
        LogError() << "";
        return ret;
    }
    ShmInstance() = (char*)shm_space;
    return 0;
}


template <class Frame>
s32 FrameDelegate<Frame>::DestroyShm(bool isUseHeap, bool self, bool force)
{
    s32 ret = 0;

    if (!self)
    {
        if (force)
        {
            s32 ret = HoldShm(isUseHeap);
            if (ret != 0)
            {
                LogError() << "";
                return ret;
            }
        }
        else
        {
            s32 ret = ResumeShm(isUseHeap);
            if (ret != 0)
            {
                LogError() << "";
                return ret;
            }
        }
    }

    if (ShmInstance() == nullptr)
    {
        LogError() << "";
        return -1;
    }

    if (!force)
    {
        DestroyObject(space<Frame, ShmSpace::kMainFrame>());
    }

    ret = zshm_boot::destroy_frame(SpaceEntry());
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }
    return 0;
}

#endif