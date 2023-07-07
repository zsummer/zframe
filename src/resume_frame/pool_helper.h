
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef POOL_HELPER_H_
#define POOL_HELPER_H_

#include "frame_def.h"
#include "frame_option.h"


/*
* 辅助注册对象池: 从配置预生成对象池头部空间, 并计算空间信息    
* 临时对象 不直接放入ShmSpace  
*/


class PoolHelper
{
private:
    PoolSpace* space_ = nullptr;
public:

    s32 Attach(PoolSpace& space, bool reset)
    {
        space_ = &space;
        if (space_ != nullptr && reset)
        {
            memset(space_, 0, sizeof(space));
        }

        s32 ret = space_->symbols_.attach(space_->names_, kLimitObjectNameBuffSize, space_->symbols_.exploit_);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }

    PoolSpace* space() const { return space_; }

    s32 Add(s32 pool_id, s32 obj_size, u64 obj_vptr, s32 obj_count, const std::string& name)
    {
        if (pool_id < 0 || pool_id >= kLimitObjectCount)
        {
            //invalid param
            return -1;
        }
        if (space_ == nullptr)
        {
            return -2;
        }

        PoolConf& conf = space_->conf_[pool_id];
        if (conf.obj_count_ > 0)
        {
            return -2;
        }
        s32 name_id = space_->symbols_.add(name.c_str(), (s32)name.length(), false);
        if (name_id == zsymbols::INVALID_SYMBOLS_ID)
        {
            //maybe names size too small.  
            return -3;
        }


        conf.obj_size_ = obj_size;
        conf.name_id_ = name_id;
        conf.vptr_ = obj_vptr;
        conf.obj_count_ = obj_count;
        conf.space_size_ = zmem_pool::calculate_space_size(obj_size, obj_count);
        if (space_->max_used_id_ < pool_id)
        {
            space_->max_used_id_ = pool_id;
        }
        return 0;
    }


    template<class _Ty>
    s32 Add(s32 pool_id, s32 obj_count, const std::string& specify_name = "")
    {
        if (pool_id < 0 || pool_id >= kLimitObjectCount)
        {
            //invalid param
            return -1;
        }
        std::string name = specify_name;
        if (name.empty())
        {
            name = zsymbols::readable_class_name<_Ty>();
        }
        s32 obj_size = (s32)sizeof(_Ty);
        u64 vptr = zmem_pool::get_vptr<_Ty>();
        s32 ret = Add(pool_id, obj_size, vptr, obj_count, name);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }
    
    s64 TotalSpaceSize() const 
    {
        s64 total_space_size = 0;
        for (s32 i = 0; i <= space_->max_used_id_; i++)
        {
            total_space_size += space_->conf_[i].space_size_;
        }
        return total_space_size;
    }

    s32 Diff(const PoolSpace* other)
    {
        if (other == nullptr || space_ == nullptr)
        {
            return -1;
        }

        int ret = memcmp(&other->conf_, &space_->conf_, sizeof(PoolConf) * kLimitObjectCount);
        if (ret != 0)
        {
            return ret;
        }

        if (space_->max_used_id_ != other->max_used_id_)
        {
            return space_->max_used_id_ - other->max_used_id_;
        }
        ret = memcmp(&other->names_, &space_->names_, sizeof(char) * kLimitObjectCount);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }

};


#endif