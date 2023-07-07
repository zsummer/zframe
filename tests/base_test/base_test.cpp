
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


//#include "frame_def.h"
#include "boot_frame.h"
#include "test_common.h"
#include "ztest.h"
#include "pool_foreach.h"


class Unit
{
public:
    Unit()
    {
        static u32 g_uid = 0;
        uid_ = g_uid++;
        seq_ = 0;
    }

    s32 Tick(s64 now_ms)
    {
        seq_++;
        LogDebug() << "Unit:" <<(void*)this <<":" << uid_ << " ticked:" << seq_ << ", now_ms:" << now_ms;
        return 0;
    }

private:
    u32 uid_;
    u32 seq_;
};


s32 UnitTick(void* unit, s64 now_ms)
{
    ((Unit*)unit)->Tick(now_ms);
    return 0;
}





class TestServer : public BaseFrame
{
public:
    s32 LoadConfig(const std::string& options, FrameConf& conf)
    {
        s32 ret = BaseFrame::LoadConfig(options, conf);
        if (ret != 0)
        {
            return ret;
        }

        PoolHelper helper;
        helper.Attach(conf.pool_conf_, true);
        ret = helper.Add<Unit>(0, 2);
        if (ret != 0)
        {
            return ret;
        }

        conf.space_conf_.use_heap_ = options.find("heap") != std::string::npos;
        conf.space_conf_.subs_[ShmSpace::kMainFrame].size_ = SPACE_ALIGN(sizeof(TestServer));
        conf.space_conf_.subs_[ShmSpace::kPool].size_ = kPoolSpaceHeadSize + helper.TotalSpaceSize();
        conf.space_conf_.subs_[ShmSpace::kBuddy].size_ = SPACE_ALIGN(zbuddy::zbuddy_size(kHeapSpaceOrder));
        conf.space_conf_.subs_[ShmSpace::kMalloc].size_ = SPACE_ALIGN(zmalloc::zmalloc_size());
        conf.space_conf_.subs_[ShmSpace::kHeap].size_ = SPACE_ALIGN(zbuddy_shift_size(kHeapSpaceOrder + kPageOrder));

        conf.space_conf_.whole_.size_ += SPACE_ALIGN(sizeof(conf.space_conf_));
        for (u32 i = 0; i < ZSHM_MAX_SPACES; i++)
        {
            conf.space_conf_.subs_[i].offset_ = conf.space_conf_.whole_.size_;
            conf.space_conf_.whole_.size_ += conf.space_conf_.subs_[i].size_;
        }
        return 0;
    }
    s32 Start()
    {
        s32 ret = BaseFrame::Start();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        zmalloc::instance().free_memory(zmalloc::instance().alloc_memory(1000));
        zmalloc::instance().check_panic();
        LogInfo() << "MyServer Start";
        foreachs_ = new (zmalloc::instance().alloc_memory<0>(sizeof(PoolForeachs))) PoolForeachs();
        foreachs_->add(0, 0, 2, 10, 1000, UnitTick);
        SubSpace<PoolSpace, kPool>()->pools_[0].create<Unit>();
        SubSpace<PoolSpace, kPool>()->pools_[0].create<Unit>();

        return 0;
    }
    s32 Resume()
    {
        s32 ret = BaseFrame::Resume();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        zmalloc::instance().free_memory(zmalloc::instance().alloc_memory(1000));
        zmalloc::instance().check_panic();
        LogInfo() << "MyServer Resume";
        foreachs_->resume(0, 0, 2, 10, 1000, UnitTick);
        return 0;
    }
    s32 Tick(s64 now_ms)
    {
        foreachs_->window_foreach(now_ms);
        return 0;
    }

public:
    PoolForeachs *foreachs_;
};



s32 boot_server(const std::string& option)
{

    if (option.find("start") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<TestServer>::BuildShm(option) == 0);
    }

    if (option.find("resume") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<TestServer>::ResumeShm(option) == 0);
    }

    if (option.find("exit") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<TestServer>::ExitShm(option) == 0);
    }

    if (option.find("del") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<TestServer>::DelShm(option) == 0);
    }

    return 0;
}


int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("zshm_boot");
    PROF_SET_OUTPUT(&FNLogFunc);

    std::string option;
    if (argc <= 1)
    {
        LogInfo() << "used [start stop resume hold] +- [heap] to start server test";
        return 0;
    }
    for (int i = 1; i < argc; i++)
    {
        option += argv[i];
    }
    
    LogInfo() << "option:" << option;

    ASSERT_TEST(boot_server(option) == 0);

    if (option.find("del") == std::string::npos)
    {
        for (s32 i = 0; i < 300; i++)
        {
            FrameDelegate<TestServer>::DoTick(zclock::now_ms());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TEST(boot_server("exit") == 0);
    }


    

    LogInfo() << "all test finish .";

    return 0;
}


