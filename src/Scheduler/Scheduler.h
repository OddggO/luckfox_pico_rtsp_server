#pragma once
#include "Event.h"
#include "Poller.h"
#include <vector>
#include "Timer.h"

class Scheduler
{
public:
    enum PollerType {
        SelectPoller, 
        PollPoller, 
        EpollPoller
    };
    static Scheduler* createNew();
    static Scheduler* createNew(PollerType type);
    Scheduler(PollerType type);
    ~Scheduler();

    bool isQuit() const; // 参数列表后面加const表示该函数不会修改类的成员变量
    bool addTriggerEvents(TriggerEvent* event);

    Poller* poller();
    bool addIOEvent(IOEvent* event);
    bool updateIOEvent(IOEvent* event);
    bool removeIOEvent(IOEvent* event);

    Timer::Timer_ID addTimerEventRunEvery(TimerEvent* event, Timer::Timer_interval interval);
    bool removeTimerEvent(Timer::Timer_ID timerID);

    bool stop();
    void loop();

private:
    void handleTriggerEvent();

private:
    bool mQuit;
    std::vector<TriggerEvent*> mTriggerEvents;
    Poller* mPoller;
    TimerManager* mTimerManager;
};