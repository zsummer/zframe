/*
* zshm_loader License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "frame_def.h"
#include "boot_frame.h"
#include "test_common.h"
#include "ztest.h"


class MyServer : public BaseFrame
{
public:
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
};



class StressServer : public BaseFrame
{
public:
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

    LogInfo() << "all test finish .";

#ifdef WIN32
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
#endif // WIN32

    return 0;
}


