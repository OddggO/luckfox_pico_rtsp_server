#include "Thread.h"

Thread::Thread(): mArg(nullptr), mIsStop(true), mIsDetach(false)
{}

Thread::~Thread()
{
    if (!mIsStop && !mIsDetach) {
        detach();
    }
}

void Thread::start(void* arg)
{
    mArg = arg;
    mThread = std::thread(&Thread::threadRun, this);
    mIsStop = false;
}

bool Thread::stop()
{
    if (mIsStop)
        return false;
    return mIsStop = true;
}

bool Thread::detach()
{
    // joinable()函数用于判断线程是否可以被join或detach
    if (mIsStop || mIsDetach || !mThread.joinable())
        return false;
    mThread.detach();
    mIsDetach = true; // 应该在mThread.detach()后面给mIsDetach赋值，否则可能因为调用程序崩溃导致无法再次完成detach和join操作
    return true;
}

bool Thread::join()
{
    if (mIsStop || mIsDetach || !mThread.joinable())
        return false;
    mThread.join();
    return true;
}

void Thread::threadRun(void* arg)
{
    Thread* thread = (Thread*)arg;
    // thread->run(thread->mArg);
    thread->run();
    // return NULL;
    // return nullptr;
}
