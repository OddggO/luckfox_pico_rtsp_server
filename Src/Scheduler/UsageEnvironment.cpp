#include "UsageEnvironment.h"

UsageEnvironment* UsageEnvironment::createNew(Scheduler* scheduler, ThreadPool* threadPool)
{
    return new UsageEnvironment(scheduler, threadPool);
}

UsageEnvironment::UsageEnvironment(Scheduler* scheduler, ThreadPool* threadPool): mScheduler(scheduler), mThreadPool(threadPool)
{
}

UsageEnvironment::~UsageEnvironment()
{
}

Scheduler* UsageEnvironment::scheduler()
{
    return mScheduler;
}

ThreadPool* UsageEnvironment::threadPool()
{
    return mThreadPool;
}
