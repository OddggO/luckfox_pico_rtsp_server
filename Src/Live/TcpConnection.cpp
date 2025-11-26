#include "TcpConnection.h"
#include "Log.h"
#include "SocketOps.h"

TcpConnection* TcpConnection::createNew(int clientFd, UsageEnvironment* env)
{
    return new TcpConnection(clientFd, env);
}

TcpConnection::TcpConnection(int clientFd, UsageEnvironment* env): mClientFd(clientFd), mEnv(env)
{
    mIOEvent = IOEvent::createNew(mClientFd, this);
    mIOEvent->setReadCallback(readCallback);
    mIOEvent->setWriteCallback(writeCallback);
    mIOEvent->setErrorCallback(errorCallback);
    mIOEvent->enableReadHandling();
    mEnv->scheduler()->addIOEvent(mIOEvent);
}

TcpConnection::~TcpConnection()
{
    if (mIOEvent) {
        mEnv->scheduler()->removeIOEvent(mIOEvent);
        delete mIOEvent;
    }
    sockets::close(mClientFd);
}

void TcpConnection::setDisConnectCallback(DisConnectCallback cb, void* arg)
{
    mDisconnectionCallback = cb;
    mDisconnectionCbArg = arg;
}

void TcpConnection::readCallback(void* arg)
{
    TcpConnection* conn = (TcpConnection*)arg;
    conn->handleRead();
}

void TcpConnection::writeCallback(void* arg)
{
    TcpConnection* conn = (TcpConnection*)arg;
    conn->handleWrite();
}

void TcpConnection::errorCallback(void* arg)
{
    TcpConnection* conn = (TcpConnection*)arg;
    conn->handleError();
}

void TcpConnection::handleRead()
{
    int ret = mInputBuffer.read(mClientFd);

    if (ret < 0) {
        LOGE("handleRead error, ret = %d\n", ret);
        handleDisconnect();
        return;
    }

    handleReadBytes(); // 子类实现
}

void TcpConnection::handleReadBytes()
{
    mInputBuffer.retrievalAll();
}

void TcpConnection::handleWrite()
{
    mInputBuffer.retrievalAll();
}

void TcpConnection::handleError()
{
    mInputBuffer.retrievalAll();
}

void TcpConnection::handleDisconnect()
{
    if (mDisconnectionCallback && mDisconnectionCbArg && mClientFd >= 0) {
        mDisconnectionCallback(mDisconnectionCbArg, mClientFd);
    }
}
