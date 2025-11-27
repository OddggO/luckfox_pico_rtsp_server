#pragma once
#include "Poller.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

class SelectPoller: public Poller
{
public:
    static SelectPoller* createNew();
    SelectPoller();
    ~SelectPoller();

    virtual bool addEvent(IOEvent* event);
    virtual bool updateEvent(IOEvent* event);
    virtual bool removeEvent(IOEvent* event);
    void handleEvent();
private:
    fd_set mReadSet;
    fd_set mWriteSet;
    fd_set mErrorSet;

    int mMaxFd;
    timeval mTimeVal;
};