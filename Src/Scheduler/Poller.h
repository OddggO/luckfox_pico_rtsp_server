#pragma once
#include "Event.h"
#include <map>
#include <vector>
#define POLLER_TIMOUT_SEC 100 // 各种Poller阻塞的秒数
#define POLLER_TIMOUT_MSEC 0 // 各种Poller阻塞的毫秒数，总阻塞时间是两个相加

class Poller
{
public:
    Poller() = default;
    virtual ~Poller() = default;

    virtual bool addEvent(IOEvent* event) = 0;
    virtual bool updateEvent(IOEvent* event) = 0;
    virtual bool removeEvent(IOEvent* event) = 0; // 如果map没有，也返回false

    virtual void handleEvent() = 0;
protected:
    typedef std::map<int, IOEvent*> EventMap;
    EventMap mEventMap;
    std::vector<IOEvent*> mIOEvents; // 临时的活跃事件
};