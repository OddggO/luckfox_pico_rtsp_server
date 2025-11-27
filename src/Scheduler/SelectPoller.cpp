#include "SelectPoller.h"
#include "Log.h"
#include <string.h>

SelectPoller* SelectPoller::createNew()
{
    return new SelectPoller();
}

SelectPoller::SelectPoller(): Poller(), mMaxFd(1)
{
    LOGI("SelectPoller()");
    FD_ZERO(&mReadSet);
    FD_ZERO(&mWriteSet);
    FD_ZERO(&mErrorSet);

    mTimeVal.tv_sec = POLLER_TIMOUT_SEC; // 1000秒
    // mTimeVal.tv_usec = POLLER_TIMOUT_MSEC * 1000;
    // mTimeVal.tv_usec = POLLER_TIMOUT_MSEC;
    mTimeVal.tv_usec = 0;

    mEventMap.clear();
    mIOEvents.clear();
}

SelectPoller::~SelectPoller()
{
    LOGI("~SelectPoller()");
}

bool SelectPoller::addEvent(IOEvent* event)
{
    return updateEvent(event);
}

bool SelectPoller::updateEvent(IOEvent* event)
{
    if (!event) {
        LOGI("event is nullptr");
        return false;
    }
    int fd = event->fd();
    if (fd < 0) {
        LOGI("fd < 0");
        return false;
    }
    FD_CLR(fd, &mReadSet);
    FD_CLR(fd, &mWriteSet);
    FD_CLR(fd, &mErrorSet);

    if (event->isReadHandling()) {
        FD_SET(fd, &mReadSet);
    }
    if (event->isWriteHandling()) {
        FD_SET(fd, &mWriteSet);
    }
    if (event->isErrorHandling()) {
        FD_SET(fd, &mErrorSet);
    }
    if (mEventMap.find(fd) == mEventMap.end()) {
        LOGI("insert fd %d into mEventMap", fd);
        mEventMap.insert(std::make_pair(fd, event));

    }
    mMaxFd = mEventMap.empty() ? 0 : mEventMap.rbegin()->first + 1;
    LOGI("update mMaxFd=%d", mMaxFd);
    return true;
}

bool SelectPoller::removeEvent(IOEvent* event)
{
    if (!event) {
        LOGI("eventt is nullptr, removeEvent error");
        return false;
    }
    int fd = event->fd();
    if (fd < 0 || mEventMap.find(fd) == mEventMap.end())
        return false;
    FD_CLR(fd, &mReadSet);
    FD_CLR(fd, &mWriteSet);
    FD_CLR(fd, &mErrorSet);
    mEventMap.erase(fd);
    mMaxFd = mEventMap.empty() ? 0 : mEventMap.rbegin()->first + 1;
    return true;
}

void SelectPoller::handleEvent()
{
    fd_set readSet = mReadSet;
    fd_set writeSet = mWriteSet;
    fd_set errorSet = mErrorSet;
    struct timeval tv = mTimeVal;
    // q: select的返回值是什么？a: 大于0时，表示有多少个文件描述符准备好了；等于0时，表示超时；小于0时，表示出错。
    int ret = select(mMaxFd, &readSet, &writeSet, &errorSet, &tv);
    if (ret < 0) {
        LOGE("select error: errno=%d(%s)", errno, strerror(errno));
        return;
    } 
    // LOGI("select return %d", ret);
    int rEventType = 0;
    for(auto it : mEventMap)
    {
        rEventType = 0;
        if (FD_ISSET(it.first, &readSet)) {
            rEventType |= IOEvent::ReadEvent;
        }
        if (FD_ISSET(it.first, &writeSet)) {
            rEventType |= IOEvent::WriteEvent;
        }
        if (FD_ISSET(it.first, &errorSet)) {
            rEventType |= IOEvent::ErrorEvent;
        }
        if (rEventType != 0) {
            it.second->setREventType((IOEvent::IOEventType)rEventType);
            mIOEvents.push_back(it.second);
        }
    }
    for(auto& event : mIOEvents)
    {
        event->handleEventCallback();
    }
    mIOEvents.clear();
}
