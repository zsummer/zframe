
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef  BOOT_FRAME_H_
#define BOOT_FRAME_H_

#include "frame_def.h"
#include "base_frame.h"
#include "frame_option.h"


template <class Frame>  //derive from BaseFrame  
class FrameDelegate
{


private:

    static inline void* AllocLarge(u64 bytes)
    {
        bytes += zbuddy_shift_size(kPageOrder) - 1;
        u32 pages = (u32)(bytes >> kPageOrder);
        u64 page_index = SubSpace<zbuddy, ShmSpace::kBuddy>()->alloc_page(pages);
        void* addr = SubSpace<char, ShmSpace::kHeap>() + (page_index << kPageOrder);
        return addr;
    }
    static inline u64 FreeLarge(void* addr, u64 bytes)
    {
        u64 offset = (char*)addr - SubSpace<char, ShmSpace::kHeap>();
        u32 page_index = (u32)(offset >> kPageOrder);
        u32 pages = SubSpace<zbuddy, ShmSpace::kBuddy>()->free_page(page_index);
        u64 free_bytes = pages << kPageOrder;
        if (free_bytes < bytes)
        {
            LogError() << "";
        }
        return free_bytes;
    }

    template<class T, class ... Args>
    static inline void RebuildVPTR(void* addr, Args ... args) {zshm_ptr<T>(addr).fix_vptr();}
    template<class T, class ... Args>
    static inline void BuildObject(void* addr, Args ... args){new (addr) T(args...);}
    template<class T>
    static inline void DestroyObject(T* addr){addr->~T();}

public:
    static inline s32 BuildShm(bool isUseHeap);
    static inline s32 ResumeShm(bool isUseHeap);
    static inline s32 DoTick(s64 now_ms);
    static inline s32 HoldShm(bool isUseHeap);
    static inline s32 DestroyShm(bool isUseHeap, bool self, bool force);
};




template <class Frame>
s32 FrameDelegate<Frame>::BuildShm(bool isUseHeap)
{
    FrameConf conf;
    s32 ret = Frame().Config(conf);
    if (ret != 0)
    {
        LogError();
        return ret;
    }

    if (true)
    {
        zshm_boot booter;
        zshm_space* shm_space = nullptr;
        ret = booter.build_frame(conf.space_conf_, shm_space);
        if (ret != 0 || shm_space == nullptr)
        {
            LogError() << "build_frame error. shm_space:" << (void*)shm_space << ", ret:" << zshm_errno::str(ret);
            return ret;
        }
        g_shm_space = shm_space;
        ShmSpace().fixed_ = (u64)shm_space;
    }


    if (true)
    {
        BuildObject<Frame>(SubSpace<Frame, ShmSpace::kMainFrame>());
    }

    if (true)
    {
        PoolSpace* space = SubSpace<PoolSpace, ShmSpace::kPool>();
        memcpy(space, &conf.pool_conf_, kPoolSpaceHeadSize);
        space->symbols_.attach(space->names_, kLimitObjectNameBuffSize, kLimitObjectNameBuffSize); //rebuild symbols   
        u64 offset = kPoolSpaceHeadSize;

        for (s32 i = 0; i <= space->max_used_id_; i++)
        {
            if (space->conf_[i].obj_count_ == 0)
            {
                continue;
            }
            //rebuild pool in real SubSpace  addr  
            zmem_pool& pool = space->pools_[i];
            PoolConf& conf = space->conf_[i];
            s32 ret = pool.init(conf.obj_size_, conf.name_id_, conf.obj_count_, (char*)space + offset, conf.space_size_);
            if (ret != 0)
            {
                LogError() << "";
                return ret;
            }
            offset += conf.space_size_;
            LogDebug() << "build pool " << space->symbols_.at(pool.name_id_) << ":" << pool;
        }

        if (offset > ShmSpace().subs_[kPool].size_)
        {
            LogError();
            return -2;
        }
    }

    if (true)
    {
        zbuddy* buddy_ptr = SubSpace<zbuddy, ShmSpace::kBuddy>();
        memset(buddy_ptr, 0, conf.space_conf_.subs_[ShmSpace::kBuddy].size_);
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::build_zbuddy(buddy_ptr, conf.space_conf_.subs_[ShmSpace::kBuddy].size_, kHeapSpaceOrder, &ret);
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
    }

    if (true)
    {
        zmalloc* malloc_ptr = SubSpace<zmalloc, ShmSpace::kMalloc>();
        memset(malloc_ptr, 0, zmalloc::zmalloc_size());
        malloc_ptr->set_global(malloc_ptr);
        malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
        malloc_ptr->check_panic();
    }

    ret = SubSpace<Frame, ShmSpace::kMainFrame>()->Start();
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
    FrameConf conf;
    s32 ret = Frame().Config(conf);
    if (ret != 0)
    {
        return ret;
    }


    if (true)
    {
        zshm_boot booter;
        zshm_space* shm_space = nullptr;
        ret = booter.resume_frame(conf.space_conf_, shm_space);
        if (ret != 0 || shm_space == nullptr)
        {
            LogError() << "booter.resume_frame error. shm_space:" << (void*)shm_space << ", ret:" << zshm_errno::str(ret);
            return ret;
        }
        g_shm_space = shm_space;
        ShmSpace().fixed_ = (u64)shm_space;
    }



    
    if (true)
    {
        Frame* m = SubSpace<Frame, ShmSpace::kMainFrame>();
        RebuildVPTR<Frame>(m);
    }


    if (true)
    {
        PoolSpace* space = SubSpace<PoolSpace, ShmSpace::kPool>();
        PoolHelper helper;
        s32 ret = helper.Attach(conf.pool_conf_, false);
        if (helper.Diff(space) != 0)
        {
            LogError() << "pool version error";
            return ret;
        }

        for (s32 i = 0; i <= space->max_used_id_; i++)
        {
            //resume object vptr
            zmem_pool& pool = space->pools_[i];
            if (pool.obj_count_ == 0)
            {
                continue;
            }
            LogDebug() << "has pool " << space->symbols_.at(pool.name_id_) << pool;
        }
    }

    if (true)
    {
        zbuddy* buddy_ptr = SubSpace<zbuddy, ShmSpace::kBuddy>();
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::rebuild_zbuddy(buddy_ptr, conf.space_conf_.subs_[ShmSpace::kBuddy].size_, kHeapSpaceOrder, &ret);
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }

    }

    if (true)
    {
        zmalloc* malloc_ptr = SubSpace<zmalloc, ShmSpace::kMalloc>();
        malloc_ptr->set_global(malloc_ptr);
        malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
        malloc_ptr->check_panic();
    }


    ret = SubSpace<Frame, ShmSpace::kMainFrame>()->Resume();
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }
    return 0;
}


template <class Frame>
s32 FrameDelegate<Frame>::DoTick(s64 now_ms)
{
    return SubSpace<Frame, kMainFrame>()->Tick(now_ms);
}




template <class Frame>
s32 FrameDelegate<Frame>::HoldShm(bool isUseHeap)
{
    if (isUseHeap)
    {
        LogError() << "";
        return -1;
    }
    FrameConf conf;
    s32 ret = Frame().Config(conf);
    if (ret != 0)
    {
        return ret;
    }

    zshm_boot booter;
    zshm_space* shm_space = nullptr;
    ret = booter.resume_frame(conf.space_conf_, shm_space);
    if (ret != 0 || shm_space == nullptr)
    {
        LogError() << "";
        return ret;
    }
    g_shm_space = shm_space;
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

    if (g_shm_space == nullptr)
    {
        LogError() << "";
        return -1;
    }

    if (!force)
    {
        DestroyObject(SubSpace<Frame, ShmSpace::kMainFrame>());
    }

    ret = zshm_boot::destroy_frame(ShmSpace());
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }
    return 0;
}

#endif