
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef POOL_FOREACH_H_
#define POOL_FOREACH_H_

#include "frame_def.h"
#include "frame_option.h"




using PoolHook = s32(*)(void*, s64);


class ForeachInst
{
public:
    inline s32 hook(const zforeach_impl::subframe& sub, u32 begin_id, u32 end_id, s64 now_ms)
    {
        PoolSpace* space = SubSpace<PoolSpace, kPool>();
        for (u32 i = begin_id; i < end_id; i++)
        {
            hook_(space->pools_[pool_id_].fixed(i), now_ms);
        }
        return 0;
    }
    s32 pool_id_;
    PoolHook hook_;
};
using PoolForeach = zforeach<ForeachInst>;




template<u32 MaxForeachs = 100>
class PoolForeachs
{
public:

    inline s32 add(u32 pool_id, u32 begin_id, u32 end_id, u32 base_frame_len, u32 long_frame_len, PoolHook hook)
    {
        if (foreachs_.full())
        {
            return -1;
        }
        PoolForeach f;
        f.foreach_inst_.pool_id_ = pool_id;
        f.foreach_inst_.hook_ = hook;
        foreachs_.push_back(f);
        s32 ret = foreachs_.back().init(0, begin_id, end_id, base_frame_len, long_frame_len);
        return ret;
    }


    inline s32 window_foreach(s64 now_ms)
    {
        for (PoolForeach& f : foreachs_)
        {
            f.window_foreach(f.subframe_.soft_begin_, f.subframe_.soft_end_, now_ms);
        }
        return 0;
    }

private:
    zarray<PoolForeach, MaxForeachs> foreachs_;
};

#endif