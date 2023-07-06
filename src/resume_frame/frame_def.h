
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/

#ifndef FRAME_DEF_H_
#define FRAME_DEF_H_


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



#ifndef FN_LOG_MAX_CHANNEL_SIZE
#define FN_LOG_MAX_CHANNEL_SIZE 4
#endif

#ifndef FN_LOG_MAX_LOG_SIZE
#define FN_LOG_MAX_LOG_SIZE 10000
#endif

//默认的全局实例ID   
#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

//PROF保留条目 用于记录一些通用的环境性能数据   
#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 

//PROF主要条目 需要先注册名字和层级关联关系  
#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 

//这两个头文件需要宏定义配置  
//不允许其他文件直接include  
#include "fn_log.h"
#include "zprof.h"



//预编译头文件  
#include <memory>

//非业务头文件并且长期不会修改  
#include "zmem_color.h"
#include "zallocator.h"
#include "zarray.h"
#include "zlist.h"
#include "zlist_ext.h"


#include "zbuddy.h"
#include "zmalloc.h"


#include "zshm_ptr.h"
#include "zshm_loader.h"
#include "zshm_boot.h"

#include "zmem_pool.h"
#include "zforeach.h"
#include "zsymbols.h"
#include "zclock.h"


inline FNLog::LogStream& operator <<(FNLog::LogStream& ls, zmem_pool& pool)
{
    ls << "[size:" << pool.obj_size_ << ", max:" << pool.obj_count_ << ", used:" << pool.used_count_ << "]";
    return ls;
}

#endif