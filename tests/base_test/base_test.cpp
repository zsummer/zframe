
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zframe, used MIT License.
*/


//#include "frame_def.h"
#include "boot_frame.h"
#include "test_common.h"
#include "ztest.h"


class MyServer : public BaseFrame
{
public:
    s32 Config(FrameConf& conf)
    {
        s32 ret = BaseFrame::Config(conf);
        if (ret != 0)
        {
            return ret;
        }
        conf.space_conf_.subs_[kMainFrame].size_ = sizeof(MyServer);
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
        LogInfo() << "MyServer Start";



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
        LogInfo() << "MyServer Resume";
        return 0;
    }
    s32 Tick(s64 now_ms)
    {
        return 0;
    }
};



class StressServer : public BaseFrame
{
public:
    s32 Config(FrameConf& conf)
    {
        s32 ret = BaseFrame::Config(conf);
        if (ret != 0)
        {
            return ret;
        }
        conf.space_conf_.subs_[kMainFrame].size_ = sizeof(StressServer);
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
        LogInfo() << "MyServer Start";

        zmalloc::instance().free_memory(zmalloc::instance().alloc_memory(1000));

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
        LogInfo() << "MyServer Resume";
        zmalloc::instance().free_memory(zmalloc::instance().alloc_memory(1000));
        return 0;
    }
    s32 Tick(s64 now_ms)
    {
        return 0;
    }
};


s32 boot_server(const std::string& option)
{
    bool use_heap = false;
    if (option.find("heap") != std::string::npos)
    {
        use_heap = true;
    }

    if (option.find("stress") != std::string::npos)
    {
        if (option.find("start") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::BuildShm(use_heap) == 0);
        }
        if (option.find("stop") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::DestroyShm(use_heap, false, true) == 0);
        }

        if (option.find("resume") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::ResumeShm(use_heap) == 0);
        }

        if (option.find("hold") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::HoldShm(use_heap) == 0);
        }

    }
    else
    {
        if (option.find("start") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::BuildShm(use_heap) == 0);
        }
        if (option.find("stop") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::DestroyShm(use_heap, false, true) == 0);
        }

        if (option.find("resume") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::ResumeShm(use_heap) == 0);
        }

        if (option.find("hold") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::HoldShm(use_heap) == 0);
        }

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

    if (option.find("stress") != std::string::npos)
    {
        for (s32 i = 0; i < 100; i++)
        {
            FrameDelegate<StressServer>::DoTick(zclock::now_ms());
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
    }
    else
    {
        for (s32 i = 0; i < 100; i++)
        {
            FrameDelegate<MyServer>::DoTick(zclock::now_ms());
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
    }


    LogInfo() << "all test finish .";



    return 0;
}


