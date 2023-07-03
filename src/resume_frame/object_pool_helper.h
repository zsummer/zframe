
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


#ifndef _OBJECT_POOL_HELPER_H_
#define _OBJECT_POOL_HELPER_H_

#include "frame_def.h"
#include "zmem_pool.h"
#include "zsymbols.h"
#include "zbitset.h"

constexpr static s32 kLimitObjectCount = 100;
constexpr static s32 kLimitObjectNameBuffSize = kLimitObjectCount * 30;


struct ObjectPoolVersion
{
    s32 obj_size_;
    s32 obj_count_;
    s32 obj_name_;
};


struct ObjectPoolHead
{
    //used check versions  
    s32 pool_max_used_id_;
    ObjectPoolVersion versions_[kLimitObjectCount];
    char object_names_[kLimitObjectNameBuffSize];
    
    //runtime states
    zmem_pool pools_[kLimitObjectCount];
    zbitset bitsets_[kLimitObjectCount];
    zsymbols symbols_;
};


constexpr static s64 kPoolHeadSize = sizeof(ObjectPoolHead);


class ObjectPoolHelper
{
public:
    ObjectPoolHelper()
    {
        head_ = new ObjectPoolHead();
        memset(head_, 0, sizeof(ObjectPoolHead));
        symbols_.attach(head_->object_names_, kLimitObjectNameBuffSize, 0);
    }
    ~ObjectPoolHelper()
    {
        delete head_;
    }

    ObjectPoolHead* head() const { return head_; }

    s32 AddPool(s32 pool_id, s32 obj_size, s32 obj_count, const std::string& name)
    {
        if (pool_id < 0 || pool_id >= kLimitObjectCount)
        {
            //invalid param
            return -1;
        }
        
        zmem_pool& pool = head_->pools_[pool_id];
        zbitset& bitset = head_->bitsets_[pool_id];
        if (pool.chunk_size() > 0)
        {
            //aready init  
            return -2;
        }
        s32 name_id = symbols_.add(name.c_str(), (s32)name.length(), false);
        if (name_id == zsymbols::invalid_symbols_id)
        {
            //maybe names size too small.  
            return -3;
        }
        pool.user_size_ = obj_size;
        pool.user_name_ = name_id;
        pool.chunk_count_ = obj_count;
        pool.space_size_ = zmem_pool::calculate_space_size(obj_size, obj_count);

        if (head_->pool_max_used_id_ < pool_id)
        {
            head_->pool_max_used_id_ = pool_id;
        }
        bitset.attach((u64*)8, zbitset::ceil_array_size(obj_size), false);
        head_->versions_->obj_size_ = obj_size;
        head_->versions_->obj_count_ = obj_count;
        head_->versions_->obj_name_ = name_id;

        return 0;
    }

    template<class _Ty>
    s32 AddPool(s32 pool_id, s32 obj_count, const std::string& specify_name = "")
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
        s32 ret = AddPool(pool_id, obj_size, obj_count, name);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }
    
    s64 TotalSpaceSize() const 
    {
        s64 total_space_size = 8;
        for (s32 i = 0; i <= head_->pool_max_used_id_; i++)
        {
            total_space_size += head_->pools_[i].space_size_;
            total_space_size += head_->bitsets_[i].array_size() * zbitset::kByteSize;
        }
        return total_space_size;
    }

    s32 CheckVersion(const ObjectPoolHead * target)
    {
        int ret = memcmp(&target->versions_, &head_->versions_, sizeof(ObjectPoolVersion) * kLimitObjectCount);
        if (ret != 0)
        {
            return ret;
        }
        if (head_->pool_max_used_id_ != target->pool_max_used_id_)
        {
            return head_->pool_max_used_id_ - target->pool_max_used_id_;
        }
        ret = memcmp(&target->object_names_, &head_->object_names_, sizeof(char) * kLimitObjectCount);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }

private:
    zsymbols symbols_;
    ObjectPoolHead* head_;
};


#endif