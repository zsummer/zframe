/*
* zshm_boot License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#ifndef  _BOOT_FRAME_H_
#define _BOOT_FRAME_H_


#ifndef ZBASE_SHORT_TYPE
#define ZBASE_SHORT_TYPE
using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
#endif

#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif

#include "fn_log.h"
#include "zprof.h"
#include "zshm_loader.h"
#include <memory>
#include <zarray.h>
#include "zshm_boot.h"
#include "zbuddy.h"
#include "zmalloc.h"
#include "zshm_ptr.h"


using shm_header = zarray<u32, 100>;

static constexpr u32 kPageOrder = 20; //1m  
static constexpr u32 kDynSpaceOrder = 8; // 8:256,  10:1024  

#define SPACE_ALIGN(bytes) zmalloc_align_value(bytes, 16)

enum  ShmSpace : u32
{
    kFrame = 0,
    kObjectPool,
    kBuddy,
    kMalloc,
    kDyn,
};







template <class Frame>  //derive from BaseFrame  
class FrameDelegate
{
public:
    static_assert(std::is_base_of<BaseFrame, Frame>::value, "");
public:
    static Frame& Instance()
    {
        return *space<Frame, ShmSpace::kFrame>();
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
        void* addr = space<char, ShmSpace::kDyn>() + (page_index << kPageOrder);
        return addr;
    }
    static inline u64 FreeLarge(void* addr, u64 bytes)
    {
        u64 offset = (char*)addr - space<char, ShmSpace::kDyn>();
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
    static inline s32 InitSpaceFromConfig(zshm_space_entry& params, bool isUseHeap)
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

        params.spaces_[ShmSpace::kFrame].size_ = SPACE_ALIGN(sizeof(Frame));
        params.spaces_[ShmSpace::kBuddy].size_ = SPACE_ALIGN(zbuddy::zbuddy_size(kDynSpaceOrder));
        params.spaces_[ShmSpace::kMalloc].size_ = SPACE_ALIGN(zmalloc::zmalloc_size());
        params.spaces_[ShmSpace::kDyn].size_ = SPACE_ALIGN(zbuddy_shift_size(kDynSpaceOrder + kPageOrder));

        params.whole_space_.size_ += SPACE_ALIGN(sizeof(params));
        for (u32 i = 0; i < ZSHM_MAX_SPACES; i++)
        {
            params.spaces_[i].offset_ = params.whole_space_.size_;
            params.whole_space_.size_ += params.spaces_[i].size_;
        }
        return 0;
    }


    static inline s32 BuildShm(bool isUseHeap)
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

        BuildObject<Frame>(space<Frame, ShmSpace::kFrame>());
        zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
        memset(buddy_ptr, 0, params.spaces_[ShmSpace::kBuddy].size_);
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::build_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kDynSpaceOrder, &ret);
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


        ret = space<Frame, ShmSpace::kFrame>()->Start();
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }

        return 0;
    }

    static inline s32 ResumeShm(bool isUseHeap)
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
        Frame* m = space<Frame, ShmSpace::kFrame>();
        RebuildVPTR<Frame>(m);


        zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::rebuild_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kDynSpaceOrder, &ret);
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }

        zmalloc* malloc_ptr = space<zmalloc, ShmSpace::kMalloc>();
        malloc_ptr->set_global(malloc_ptr);
        malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
        malloc_ptr->check_health();

        ret = space<Frame, ShmSpace::kFrame>()->Resume();
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
        return 0;
    }

    static inline s32 HoldShm(bool isUseHeap)
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

    static inline s32 DestroyShm(bool isUseHeap, bool self, bool force)
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
            DestroyObject(space<Frame, ShmSpace::kFrame>());
        }

        ret = zshm_boot::destroy_frame(SpaceEntry());
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
        return 0;
    }

private:

};


#endif