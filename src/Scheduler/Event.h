#pragma once

typedef void (*EventCallback)(void*);

// TriggerEvent是对触发事件的封装，调用者在触发事件时，调用handleEventCallback()方法执行回调函数
class TriggerEvent
{
public:
    static TriggerEvent* createNew();
    static TriggerEvent* createNew(EventCallback callback, void* arg);
    TriggerEvent(EventCallback callback, void* arg);

    void setCallback(EventCallback callback);
    void setArg(void* mArg);

    void handleEventCallback();
private:
    EventCallback mEventCallback;
    void* mArg;
};

class IOEvent
{
public:
    enum IOEventType
    {
        NoneEvent = 0x00, 
        ReadEvent = 0x01, 
        WriteEvent = 0x02, 
        ErrorEvent = 0x04
    };
    static IOEvent* createNew(int fd, void* arg);
    IOEvent(int fd, void* arg);
    ~IOEvent();

    int fd() const {
        return mFd;
    }
    IOEventType eventType() const {
        return mEventType;
    }
    void setREventType(IOEventType eventType) { 
        mREventType = eventType; 
    }
    void setArg(void* arg) { 
        mArg = arg; 
    }

    void setReadCallback(EventCallback callback) {
        mReadCallback = callback;
    }
    void setWriteCallback(EventCallback callback){
        mWriteCallback = callback;
    }
    void setErrorCallback(EventCallback callback) {
        mErrorCallback = callback;
    }

    void enableReadHandling() {
        // mEventType |= ReadEvent;
        mEventType = (IOEventType)(mEventType | ReadEvent);
    }
    void enableWriteHandling() {
        mEventType = (IOEventType)(mEventType | WriteEvent);
    }
    void enableErrorHandling() {
        mEventType = (IOEventType)(mEventType | ErrorEvent);
    }

    void disableReadHandling() {
        // mEventType &= ReadEvent;
        mEventType = (IOEventType)(mEventType & (~ReadEvent));
    }
    void disableWriteHandling() {
        mEventType = (IOEventType)(mEventType & (~WriteEvent));
    }
    void disableErrorHandling() {
        mEventType = (IOEventType)(mEventType & (~ErrorEvent));
    }
    void disableAllHandling() {
        mEventType = NoneEvent;
    }

    bool isNoneHandling() const {
        return mEventType == NoneEvent;
    }
    bool isReadHandling() const {
        return mEventType & ReadEvent;
    }
    bool isWriteHandling() const {
        return mEventType & WriteEvent;
    }
    bool isErrorHandling() const {
        return mEventType & ErrorEvent;
    }

    void handleEventCallback();

private:
    int mFd; // 交给io多路复用的套接字
    void* mArg;
    IOEventType mEventType; // 执行IO多路复用函数前，需要检测的事件类型
    IOEventType mREventType; // 执行IO多路复用函数后，检测到的事件类型
    EventCallback mReadCallback;
    EventCallback mWriteCallback;
    EventCallback mErrorCallback;
};

// TimerEvent其实和TriggerEvent差别不大， 就是多了个停止功能
class TimerEvent
{
public:
    static TimerEvent* createNew();
    static TimerEvent* createNew(EventCallback callback, void* arg);
    TimerEvent(EventCallback callback, void* arg);
    ~TimerEvent();

    void setCallback(EventCallback callback);
    void setArg(void* mArg);

    bool handleTimeout();
    bool stop();
private:
    void* mArg;
    EventCallback mTimeOutCallback;
    bool mIsStop;
};