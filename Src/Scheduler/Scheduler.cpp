#include "Scheduler.h"
#include "SelectPoller.h"
#include "EpollPoller.h"
#include "Log.h"

Scheduler* Scheduler::createNew()
{
    return new Scheduler(SelectPoller);
}

Scheduler* Scheduler::createNew(PollerType type)
{
    return new Scheduler(type);
}

Scheduler::Scheduler(PollerType type): mQuit(false)
{
    LOGI("Scheduler()");
    mTriggerEvents.clear();
    switch (type)
    {
    case SelectPoller:
        // mPoller = new SelectPoller(); // TODO: 为什么报错？？？
        mPoller = SelectPoller::createNew();
        break;
    case EpollPoller:
        mPoller = EpollPoller::createNew();
        break;
    default:
        LOGI("error PollerType, use the SelectPoller as default");
        mPoller = SelectPoller::createNew();
    }

    mTimerManager = TimerManager::createNew(mPoller);
}

Scheduler::~Scheduler()
{
    LOGI("~Scheduler()");
    if (mPoller) {
        delete mPoller;
        mPoller = nullptr;
    }
    if (mTimerManager) {
        delete mTimerManager;
        mTimerManager = nullptr;
    }
}

bool Scheduler::isQuit() const
{
    return mQuit;
}

bool Scheduler::addTriggerEvents(TriggerEvent* event)
{
    if (!event) {
        LOGI("event is nullptr");
        return false;
    }
    mTriggerEvents.push_back(event);
    return true;
}

void Scheduler::handleTriggerEvent()
{
    for(auto event : mTriggerEvents)
    {
        event->handleEventCallback();
    }
    mTriggerEvents.clear();
}

Poller* Scheduler::poller()
{
    return mPoller;
}

bool Scheduler::addIOEvent(IOEvent* event)
{
    return mPoller->addEvent(event);
}

bool Scheduler::updateIOEvent(IOEvent* event)
{
    return mPoller->updateEvent(event);
}

bool Scheduler::removeIOEvent(IOEvent* event)
{
    return mPoller->removeEvent(event);
}

Timer::Timer_ID Scheduler::addTimerEventRunEvery(TimerEvent* event, Timer::Timer_interval interval)
{
    // Timer::Timer_interval stamp = TimerManager::getCurTime() + interval;
    Timer::Timer_stamp stamp = TimerManager::getCurTime() + interval;
    LOGI("addTimerEventRunEvery with stamp=%ld interval=%u", stamp, interval);
    return mTimerManager->addTimer(event, stamp, interval);
}

bool Scheduler::removeTimerEvent(Timer::Timer_ID timerID)
{
    LOGI("removeTimerEvent with timerID=%u", timerID);
    return mTimerManager->removeTimer(timerID);
}

bool Scheduler::stop()
{
    if (mQuit)
        return false;
    return mQuit = true;
}

void Scheduler::loop()
{
    while (!mQuit)
    {
        handleTriggerEvent();
        mPoller->handleEvent();
    }
}
