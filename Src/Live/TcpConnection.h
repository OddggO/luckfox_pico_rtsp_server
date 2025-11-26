#pragma once
#include "UsageEnvironment.h"
#include "Buffer.h"
#include "Event.h"

class TcpConnection
{
public:
    TcpConnection* createNew(int clientFd, UsageEnvironment* env);
    TcpConnection(int clientFd, UsageEnvironment* env);
    virtual ~TcpConnection();

    typedef void (*DisConnectCallback)(void*, int);
    void setDisConnectCallback(DisConnectCallback cb, void* arg);
protected:
    static void readCallback(void* arg);
    static void writeCallback(void* arg);
    static void errorCallback(void* arg);

    void handleRead();
    virtual void handleReadBytes();
    virtual void handleWrite();
    virtual void handleError();

    void handleDisconnect();

protected:
    int mClientFd;
    UsageEnvironment* mEnv;
    IOEvent* mIOEvent;
    Buffer mInputBuffer;
    Buffer mOutputBuffer;
    DisConnectCallback mDisconnectionCallback;
    void* mDisconnectionCbArg;
    char mBuffer[2048];
};
