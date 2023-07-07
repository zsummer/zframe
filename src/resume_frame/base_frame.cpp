
#include "base_frame.h"

zshm_space* g_shm_space = nullptr;


s32 BaseFrame::Config(const std::string& options, FrameConf& conf)
{
    memset(&conf, 0, sizeof(conf));
    conf.space_conf_.shm_key_ = 198709;
    conf.space_conf_.use_heap_ = false;
#ifdef WIN32

#else
    conf.space_conf_.use_fixed_ = 1;
    conf.space_conf_.fixed_ = 0x00006AAAAAAAAAAAULL;
    conf.space_conf_.fixed_ = 0x0000700000000000ULL;
#endif // WIN32

    PoolHelper helper;
    helper.Attach(conf.pool_conf_, true);
    s32 ret = helper.Add(23, 10, 0, 100, "test");
    if (ret != 0)
    {
        return ret;
    }


    conf.space_conf_.subs_[ShmSpace::kMainFrame].size_ = SPACE_ALIGN(sizeof(BaseFrame));
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


s32 BaseFrame::Init()
{
    return 0;
}

s32 BaseFrame::Start()
{
    return 0;
}

s32 BaseFrame::Resume()
{
    return 0;
}





