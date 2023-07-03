
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef _OBJECT_POOL_HELP_H_
#define _OBJECT_POOL_HELP_H_

#include "frame_def.h"


constexpr static s32 kLimitObjectCount = 100;

struct ObjectPoolOption
{
    u32 type_id_;
    u32 type_size_;
    u32 type_name_id_;
    u32 object_count_;
};


struct ObjectPoolHead
{

};

class ObjectPoolHelper
{
public:
    
};


#endif