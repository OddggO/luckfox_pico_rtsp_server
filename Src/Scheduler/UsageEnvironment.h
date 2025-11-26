#pragma once
#include "Scheduler.h"
#include "ThreadPool.h"

class UsageEnvironment
{
public:
    static UsageEnvironment* createNew(Scheduler* scheduler, ThreadPool* threadPool);
    UsageEnvironment(Scheduler* scheduler, ThreadPool* threadPool);
    ~UsageEnvironment();

    Scheduler* scheduler();
    ThreadPool* threadPool();
private:
    Scheduler* mScheduler;
    ThreadPool* mThreadPool;
};