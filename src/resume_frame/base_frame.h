
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef BASE_FRAME_H_
#define BASE_FRAME_H_
#include "frame_option.h"
#include "pool_helper.h"


class BaseFrame
{
public:
    virtual s32 Config(FrameConf& conf);
    virtual s32 Init();
    virtual s32 Resume();

    virtual s32 Start();
    

    virtual s32 Tick(s64 now_ms) = 0;

private:

};


#endif