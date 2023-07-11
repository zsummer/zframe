
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef  FRAME_BOOT_H_
#define FRAME_BOOT_H_

#include "frame_def.h"
#include "base_frame.h"
#include "frame_option.h"


template <class Frame>  //derive from BaseFrame  
//requires std::is_base_of<BaseFrame, Frame>::value
class FrameBoot
{
private:
    static_assert(std::is_base_of<BaseFrame, Frame>::value, "need base of BaseFrame");
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
    static inline s32 BuildShm(const std::string& options);
    static inline s32 ResumeShm(const std::string& options);
    static inline s32 ExitShm(const std::string& options); //正常退出并清理 
    static inline s32 DelShm(const std::string& options);
    static inline s32 DoTick(s64 now_ms);
};




template <class Frame>
s32 FrameBoot<Frame>::BuildShm(const std::string& options)
{
    FrameConf conf;
    s32 ret = Frame::LoadConfig(options, conf);
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
            s32 ret = pool.init(conf.obj_size_, conf.name_id_, conf.vptr_, conf.obj_count_, (char*)space + offset, conf.space_size_);
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
        BuildObject<Frame>(SubSpace<Frame, ShmSpace::kMainFrame>());
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
s32 FrameBoot<Frame>::ResumeShm(const std::string& options)
{
    FrameConf conf;
    s32 ret = Frame::LoadConfig(options, conf);
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
        //ShmSpace().fixed_ = (u64)shm_space;
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

    if (true)
    {
        PoolSpace* space = SubSpace<PoolSpace, ShmSpace::kPool>();
        //space->symbols_.attach(space->names_, kLimitObjectNameBuffSize, kLimitObjectNameBuffSize);
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
            pool.resume(space->conf_[i].vptr_);
            LogDebug() << "has pool " << space->symbols_.at(pool.name_id_) << pool;
        }
    }

    if (true)
    {
        Frame* m = SubSpace<Frame, ShmSpace::kMainFrame>();
        RebuildVPTR<Frame>(m);
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
s32 FrameBoot<Frame>::DoTick(s64 now_ms)
{
    return SubSpace<Frame, kMainFrame>()->Tick(now_ms);
}


template <class Frame>
s32 FrameBoot<Frame>::ExitShm(const std::string& options)
{
    if (g_shm_space == nullptr)
    {
        LogError() << "";
        return -1;
    }

    DestroyObject(SubSpace<Frame, ShmSpace::kMainFrame>());

    s32 ret = zshm_boot::destroy_frame(ShmSpace());
    if (ret != 0)
    {
        LogError() << "";
        return ret;
    }
    return 0;
}



template <class Frame>
s32 FrameBoot<Frame>::DelShm(const std::string& options)
{
    s32 ret = 0;
    FrameConf conf;
    ret = Frame::LoadConfig(options, conf);
    if (ret != 0)
    {
        LogError() << "Destroy shm has error: load config error ";
        return ret;
    }

    zshm_boot booter;
    ret = booter.destroy_frame(conf.space_conf_);
    if (ret != 0)
    {
        LogError() << "Destroy shm has error:" << zshm_errno::str(ret) <<", please used ipcs -m/ ipcrm -m to destroy it. ";
        return ret;
    }
    return 0;
}

#endif