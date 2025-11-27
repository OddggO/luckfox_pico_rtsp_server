#pragma
#include "Poller.h"
#include <sys/epoll.h>
#include <vector>

class EpollPoller: public Poller
{
public:
    static EpollPoller* createNew();
    EpollPoller();
    ~EpollPoller();
    virtual bool addEvent(IOEvent* event);
    virtual bool updateEvent(IOEvent* event);
    virtual bool removeEvent(IOEvent* event);

    virtual void handleEvent();
private:
    int mEpfd;
    struct epoll_event mEv;
    struct epoll_event mEvs[1024];
    int mEvsSize = sizeof (mEvs) / sizeof(struct epoll_event);
};
