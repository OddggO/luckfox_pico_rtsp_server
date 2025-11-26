#pragma once
#include "Event.h"
#include <stdint.h>
// #include "Poller.h"
#include <map>

class Poller;
class TimerManager;
class Timer
{
public:
    typedef uint32_t Timer_ID;
    typedef int64_t Timer_stamp;
    typedef uint32_t Timer_interval;

    static Timer* createNew(TimerEvent* timerEvent, Timer_ID timerID, Timer_stamp timerStamp, Timer_interval timerInterval);
    Timer(TimerEvent* timerEvent, Timer_ID timerID, Timer_stamp timerStamp, Timer_interval timerInterval);
    ~Timer();

    bool handleEvent();
private:
    friend TimerManager;
    TimerEvent* mTimerEvent;
    Timer_ID mTimerID;
    Timer_stamp mTimerStamp;
    Timer_interval mTimerInterval;
    bool mRepeat;
};

/**
 * 改进：不同的Sink使用不同的定时器，而不是和现在一样使用统一的定时器
 */
class TimerManager
{
public:
    static TimerManager* createNew(Poller* poller);
    TimerManager(Poller* poller);
    ~TimerManager();

    static Timer::Timer_stamp getCurTime();

    Timer::Timer_ID addTimer(TimerEvent* event, Timer::Timer_stamp stamp, Timer::Timer_interval interval);
    bool removeTimer(Timer::Timer_ID timerID);

private:
    static void callback(void* arg);
    void handleTimeout();
    void modifyTimeout();
private:
    Poller* mPoller;
    IOEvent* mTimerEvent; // 这是一个IOEvent，而不是TimerEvent，用来监听timerfd的读事件
    int mFd;
    Timer::Timer_ID mTimerLastID;
    std::map<Timer::Timer_ID, Timer> mTimers;
    std::multimap<Timer::Timer_stamp, Timer> mEvents;
};