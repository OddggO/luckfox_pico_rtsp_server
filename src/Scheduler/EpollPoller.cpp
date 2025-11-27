#include "EpollPoller.h"

EpollPoller* EpollPoller::createNew()
{
    return new EpollPoller();
}
EpollPoller::EpollPoller(): Poller()
{
    mEpfd = epoll_create(100);
}

EpollPoller::~EpollPoller()
{
    // epoll
}

bool EpollPoller::addEvent(IOEvent* event)
{
    return updateEvent(event);
}

bool EpollPoller::updateEvent(IOEvent* event)
{
    if (!event) {
        return false;
    }
    int fd = event->fd();
    if (fd < 0) {
        return false;
    }
    mEv.data.fd = fd;
    mEv.events = 0;
    if (event->isReadHandling()) {
        mEv.events |= EPOLLIN;
    }
    if (event->isWriteHandling()) {
        mEv.events |= EPOLLOUT;
    }
    // EPOLLERR 和 EPOLLHUP 无需显式注册，但可加上以明确意图（不影响）
    if (event->isErrorHandling()) {
        mEv.events |= EPOLLERR;
    }
    // mEv.events |= EPOLLET; // 开启边缘模式
    int op = mEventMap.find(fd) == mEventMap.end() ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    if (epoll_ctl(mEpfd, op, fd, &mEv) != 0) {
        return false;
    }
    // if (mEventMap.find(fd) == mEventMap.end()) {
    //     mEventMap.insert(std::make_pair(fd, event));
    // }
    mEventMap[fd] = event; // 可以自动插入或者更新
    return true;
}

bool EpollPoller::removeEvent(IOEvent* event)
{
    if (!event) {
        return false;
    }
    int fd = event->fd();
    if (fd < 0) {
        return false;
    }
    if (epoll_ctl(mEpfd, EPOLL_CTL_DEL, fd, nullptr) != 0) {
        return false;
    }
    if (mEventMap.find(fd) != mEventMap.end()) {
        mEventMap.erase(fd);
    } 
    else {
        return false;
    }
    return true;
}

void EpollPoller::handleEvent()
{
    int num = epoll_wait(mEpfd, mEvs, mEvsSize, POLLER_TIMOUT_SEC);
    int rEventType;
    for (int i = 0; i < num; ++i)
    {
        rEventType = 0;
        int fd = mEvs[i].data.fd;
        auto it = mEventMap.find(fd);
        if (it == mEventMap.end()) {
            continue;
        }
        if (mEvs[i].events & EPOLLIN) {
            rEventType |= IOEvent::ReadEvent;
        }
        if (mEvs[i].events & EPOLLOUT) {
            rEventType |= IOEvent::WriteEvent;
        }
        if (mEvs[i].events & EPOLLERR) {
            rEventType |= IOEvent::ErrorEvent;
        }
        it->second->setREventType(IOEvent::IOEventType(rEventType));
        mIOEvents.push_back(it->second);
    }

    for (auto& it : mIOEvents)
    {
        it->handleEventCallback();
    }
    mIOEvents.clear();
}
