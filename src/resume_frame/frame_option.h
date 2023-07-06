
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef FRAME_OPTION_H_
#define FRAME_OPTION_H_
#include "frame_def.h"



//公共定制类常量尽量放在局部def/option中   
//这里修改可能导致大量文件重编译  

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






constexpr static s32 kLimitObjectCount = 100;
constexpr static s32 kLimitObjectNameBuffSize = kLimitObjectCount * 30;


struct PoolConf
{
    s32 obj_size_;
    s32 obj_count_;
    s32 name_id_;
    s64 space_size_;
};


struct PoolSpace
{
    //used check versions  
    s32 max_used_id_;
    PoolConf conf_[kLimitObjectCount];
    char names_[kLimitObjectNameBuffSize];

    //runtime states
    zmem_pool pools_[kLimitObjectCount];
    zsymbols symbols_;
};


constexpr static s64 kPoolSpaceHeadSize = sizeof(PoolSpace);




struct FrameConf
{
    zshm_space space_conf_;
    PoolSpace pool_conf_;
};



extern zshm_space* g_shm_space;

static inline zshm_space& ShmSpace()
{
    return *g_shm_space;
}




template <class T, u32 ID>
static inline T* SubSpace()
{
    return (T*)((char*)g_shm_space + ShmSpace().subs_[ID].offset_);
}



#endif