#include "Event.h"
#include "Log.h"

TriggerEvent* TriggerEvent::createNew()
{
    return new TriggerEvent(nullptr, nullptr);
}


TriggerEvent* TriggerEvent::createNew(EventCallback callback, void* arg)
{
    return new TriggerEvent(callback, arg);
}

TriggerEvent::TriggerEvent(EventCallback callback, void* arg): mEventCallback(callback), mArg(arg)
{}

void TriggerEvent::setCallback(EventCallback callback)
{
    mEventCallback = callback;
}

void TriggerEvent::setArg(void* arg)
{
    mArg = arg;
}

void TriggerEvent::handleEventCallback()
{
    if (mEventCallback)
        mEventCallback(mArg);
}

IOEvent* IOEvent::createNew(int fd, void* arg)
{
    if (fd < 0) {
        LOGI("createNew IOEvent error, fd=%d", fd);
        return nullptr;
    }
    return new IOEvent(fd, arg);
}

IOEvent::IOEvent(int fd, void* arg): mFd(fd), mArg(arg), mEventType(NoneEvent), mREventType(NoneEvent), 
                mReadCallback(nullptr), mWriteCallback(nullptr), mErrorCallback(nullptr)
{
    LOGI("IOEvent() fd=%d", fd);
}

IOEvent::~IOEvent()
{
    LOGI("~IOEvent() fd=%d", mFd);
}

void IOEvent::handleEventCallback()
{
    if ((mREventType & ReadEvent) && mReadCallback) {
        mReadCallback(mArg);
    }
    if ((mREventType & WriteEvent) && mWriteCallback) {
        mWriteCallback(mArg);
    }
    if ((mREventType & ErrorEvent) && mErrorCallback) {
        mErrorCallback(mArg);
    }
}

TimerEvent* TimerEvent::createNew()
{
    return new TimerEvent(nullptr, nullptr);
}

TimerEvent* TimerEvent::createNew(EventCallback callback, void* arg)
{
    return new TimerEvent(callback, arg);
}

TimerEvent::TimerEvent(EventCallback callback, void* arg): mTimeOutCallback(callback), mArg(arg), mIsStop(false)
{
    LOGI("TimerEvent()");
}

TimerEvent::~TimerEvent()
{
    LOGI("~TimerEvent()");
}

void TimerEvent::setCallback(EventCallback callback)
{
    mTimeOutCallback = callback;
}

void TimerEvent::setArg(void* arg)
{
    mArg = arg;
}

bool TimerEvent::handleTimeout()
{
    if (mTimeOutCallback && !mIsStop)
        mTimeOutCallback(mArg); 
    return mIsStop; // TODO 是否应该是!mIsStop
}

bool TimerEvent::stop()
{
    if (mIsStop)
        return false;
    LOGI("TimerEvent to stop");
    return mIsStop = true;
}
