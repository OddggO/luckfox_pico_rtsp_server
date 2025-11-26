#include "Timer.h"
#include <sys/timerfd.h>
#include <stdlib.h>
#include "Log.h"
#include <string.h>
#include <chrono>
#include "Poller.h"

Timer* Timer::createNew(TimerEvent* timerEvent, Timer_ID timerID, Timer_stamp timerStamp, Timer_interval timerInterval)
{
    return new Timer(timerEvent, timerID, timerStamp, timerInterval);
}

Timer::Timer(TimerEvent* timerEvent, Timer_ID timerID, Timer_stamp timerStamp, Timer_interval timerInterval)
            : mTimerEvent(timerEvent), mTimerID(timerID), mTimerStamp(timerStamp), mTimerInterval(timerInterval)
{
    if (timerInterval > 0)
        mRepeat = true;
    else
        mRepeat = false;
}

Timer::~Timer()
{}

bool Timer::handleEvent()
{
    if (!mTimerEvent)
        return false;
    return mTimerEvent->handleTimeout();
}

static bool timerFdSetTime(int fd, uint64_t when, uint32_t period)
{
    struct itimerspec newVal;
    // it_value表示定时器起点
    newVal.it_value.tv_sec = when / 1000; // ms -> s
    newVal.it_value.tv_nsec = (when % 1000) * 1000 * 1000; // ms -> us -> ns
    // it_interval表示定时器的周期性时间间隔
    newVal.it_interval.tv_sec = period / 1000;
    newVal.it_interval.tv_nsec = (period % 1000) * 1000 * 1000;
    // TFD_TIMER_ABSTIME表示设置绝对时间
    // 最后一个参数是输出参数，用与返回定时器之前设置的itimerspec值
    int oldExpirationTime = timerfd_settime(fd, TFD_TIMER_ABSTIME, &newVal, NULL); 
    if (oldExpirationTime < 0)
    {
        LOGE("timerfd_settime faild, errno=%d (%s)\n", errno, strerror(errno));
        return false;
    }
    return true;
}

TimerManager* TimerManager::createNew(Poller* poller)
{
    return new TimerManager(poller);
}

TimerManager::TimerManager(Poller* poller): mPoller(poller), mTimerLastID(0)
{
    LOGI("TimerManager()");
    /**
     * CLOCK_MONOTONIC: 单调时钟
     * TFD_NONBLOCK: 文件描述符为非阻塞模式
     * TFD_CLOEXEC: 执行exec()时自动关闭该文件描述符
     */
    mFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (mFd < 0) {
        LOGE("timerfd_create error, return: %d", mFd); 
        exit(-1);
        return;
    }
    timerFdSetTime(mFd, 0, 0); // 传入时间为0，不计时、不触发事件
    mTimerEvent = IOEvent::createNew(mFd, this);
    mTimerEvent->setReadCallback(callback);
    mTimerEvent->enableReadHandling();
    modifyTimeout();
    if (mPoller->addEvent(mTimerEvent)) {
        LOGI("add timerfd=%d to poller success", mFd);
    } else {
        LOGE("add timerfd=%d to poller failed", mFd);
    }
}

TimerManager::~TimerManager()
{
    LOGI("~TimerManager()");
    if (mTimerEvent) {
        if (mPoller->removeEvent(mTimerEvent)) {
            LOGI("remove timerfd=%d from poller success", mFd);
        } else {
            LOGE("remove timerfd=%d from poller failed", mFd);
        }
        delete mTimerEvent;
        mTimerEvent = nullptr;
    }
}

// 单调时钟，不随系统时间调整而改变，返回时间戳，即从1970-01-01 00:00:00 UTC到现在的毫秒数
Timer::Timer_stamp TimerManager::getCurTime()
{
#ifdef WIN32    
    return std::chrono::steady_clock::now().time_since_epoch().count() / 1000000;
#else
    struct timespec now;
    // CLOCK_MONOTONIC是单调时钟（不受系统时间调整影响），基本等价于std::chrono::steady_clock
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000; 
#endif // WIN32    
}

Timer::Timer_ID TimerManager::addTimer(TimerEvent* event, Timer::Timer_stamp stamp, Timer::Timer_interval interval)
{
    if (!event)
        return false;
    
    ++mTimerLastID;
    Timer timer = Timer(event, mTimerLastID, stamp, interval);
    mTimers.insert(std::make_pair(mTimerLastID, timer));
    mEvents.insert(std::make_pair(stamp, timer));
    modifyTimeout();
    return mTimerLastID;
}

bool TimerManager::removeTimer(Timer::Timer_ID timerID)
{
    auto it = mTimers.find(timerID);
    if (it == mTimers.end()) {
        LOGI("cannot find timerID=%u", timerID);
        return false;
    }
    // it->second.mTimerEvent->stop(); // 是否要自动设置Timer的事件暂停？还是交给Sink控制？
    it = mTimers.erase(it);
    LOGI("remove timerID=%u, stamp=%ld, interval=%u, mTimers size=%zu", 
         timerID, it->second.mTimerStamp, it->second.mTimerInterval, mTimers.size());

    modifyTimeout();
    return true;
}

void TimerManager::callback(void* arg)
{
    TimerManager* manager = (TimerManager*)arg;
    manager->handleTimeout();
}

/**
 * TimerManager是为每个Sink设置了统一的定时器，然后频繁调用modifyTimeout修改下次timeout的时间，
 * 这可能是音画不同步的一大原因
 */
void TimerManager::handleTimeout()
{
    if (mTimers.empty() || mEvents.empty()) {
        LOGI("no timers exist");
        return;
    }
    Timer::Timer_stamp curStamp = getCurTime();
    auto it = mEvents.begin();
    Timer timer = it->second;
    int expire = curStamp - it->first;
    // 如果TimerEvent没有停止，且Timer是周期性的，则再次将Timer插入mEvents末尾
    if (expire >= 0) {
        bool isEventStop = it->second.handleEvent();
        // TODO q: erase后it是否有效？a: 有效，指向下一个元素
        it = mEvents.erase(it); 
        if (timer.mRepeat && !isEventStop) {
            // it->second.mTimerStamp = curStamp + it->second.mTimerInterval; // 更新时间起点
            timer.mTimerStamp = curStamp + timer.mTimerInterval; // 更新时间起点
            // mEvents.emplace(it); // 如果用位置it提高效率，用mEvents.emplace_hint(it, key, timer);
            // mEvents.emplace_hint(it, it->first, it->second);
            // q: 插入it还是插入timer？为什么？a: 插入timer，因为it不是erased之前的it了
            // mEvents.emplace(it->first, it->second); 
            mEvents.insert(std::make_pair(timer.mTimerStamp, timer));
        } else {
            // mTimers.erase(it->second.mTimerID);
            mTimers.erase(timer.mTimerID);
        }
    } 
    // else {
    //     mTimers.erase(it->second.mTimerID);
    // }
    modifyTimeout();
}

void TimerManager::modifyTimeout()
{
    if (mEvents.empty()) {
        timerFdSetTime(mFd, 0, 0);
        return;
    }
    auto it = mEvents.begin(); // 使用最早timeout的Timer设置定时器的起点和周期间隔
    timerFdSetTime(mFd, it->second.mTimerStamp, it->second.mTimerInterval);    
}
