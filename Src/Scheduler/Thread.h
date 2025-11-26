#pragma once
#include <thread>

class Thread
{
public:

    Thread();
    virtual ~Thread();
    void start(void* arg);
    bool stop();
    bool detach();
    bool join();
protected:
    // 这是为了兼容pthread设计的入口函数，实际上如果只用std::thread，返回值可以改为void
    // static void* threadRun(void* arg);
    static void threadRun(void* arg);
    // virtual void run(void* arg) = 0;
    virtual void run() = 0;
// private:
protected:
    std::thread mThread;
    void* mArg;
    bool mIsStop;
    bool mIsDetach;
};