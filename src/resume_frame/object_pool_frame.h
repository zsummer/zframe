
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef _OBJECT_POOL_FRAME_H_
#define _OBJECT_POOL_FRAME_H_

#include "frame_def.h"
#include "object_pool_helper.h"





#define SPACE_ALIGN(bytes) zmalloc_align_value(bytes, 16)


class ObjectPoolFrame
{
public:
    static ObjectPoolFrame& Instance()
    {
        return *ObjectPoolFrame::ShmInstance();
    }

    static inline ObjectPoolFrame*& ShmInstance()
    {
        static ObjectPoolFrame* g_instance_ptr = nullptr;
        return g_instance_ptr;
    }
};


#endif