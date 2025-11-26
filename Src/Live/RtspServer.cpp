#include "RtspServer.h"
#include "RtspConnection.h"
#include "Log.h"
#include <string.h>
#include <fcntl.h>

RtspServer* RtspServer::createNew(UsageEnvironment* env, MediaSessionManager* sessionManager, const std::string& ip, uint16_t port)
{
    return new RtspServer(env, sessionManager, ip, port);
}

RtspServer::RtspServer(UsageEnvironment* env, MediaSessionManager* sessionManager, const std::string& ip, uint16_t port): 
            mEnv(env), mSessionManager(sessionManager), mIpv4Address(ip, port), mListen(false)
{
    mFd = sockets::createTcpSocket();
    if (mFd < 0) {
        LOGE("createTcpSocket failed, fd = %d", mFd);
    }
    // 设置复用
    sockets::reUseAddr(mFd);
    if (!sockets::bind(mFd, mIpv4Address.getIp(), mIpv4Address.getPort())) {
        return;
    }
    LOGI("rtsp://%s:%d fd=%d", mIpv4Address.getIp().c_str(), mIpv4Address.getPort(), mFd);
    
    mAcceptEvent = IOEvent::createNew(mFd, this);
    mAcceptEvent->setReadCallback(acceptCallback);
    mAcceptEvent->enableReadHandling();

    mCloseConnectEvent = TriggerEvent::createNew(closeConnectCallback, this);
}

RtspServer::~RtspServer()
{
    if (mListen)
        mEnv->scheduler()->removeIOEvent(mAcceptEvent);
    delete mAcceptEvent;
    delete mCloseConnectEvent;

    sockets::close(mFd);
}

void RtspServer::start()
{
    mEnv->scheduler()->addIOEvent(mAcceptEvent);
    sockets::listen(mFd, 60);
    mListen = true;
}

UsageEnvironment* RtspServer::env() const
{
    return mEnv;
}

MediaSessionManager* RtspServer::mediaSessionManager()
{
    return mSessionManager;
}

void RtspServer::acceptCallback(void* arg)
{
    RtspServer* server = (RtspServer*)arg;
    server->handleAccept();
}

void RtspServer::handleAccept()
{
    int cfd = sockets::accept(mFd);
    if (cfd < 0) {
        LOGE("accept client listen failed, cfd: %d, errno=%d(%s)\n", cfd, errno, strerror(errno));
        LOGE("acceptCallback: mFd=%d, mFd_address=%p, fcntl=%d", mFd, &mFd, fcntl(mFd, F_GETFD));
        return;
    }
    RtspConnection* conn = RtspConnection::createNew(cfd, this);
    conn->setDisConnectCallback(RtspServer::disConnectCallback, this);
    mConnMap.insert(std::make_pair(cfd, conn));
}

void RtspServer::disConnectCallback(void* arg, int clientfd)
{
    RtspServer* server = (RtspServer*)arg;
    server->handleDisConnect(clientfd);
}

void RtspServer::handleDisConnect(int clientfd)
{
    std::lock_guard<std::mutex> lck(mMtx);
    mDisConns.push_back(clientfd);
    mEnv->scheduler()->addTriggerEvents(mCloseConnectEvent);
}

void RtspServer::closeConnectCallback(void* arg)
{
    RtspServer* server = (RtspServer*)arg;
    server->handleCloseConnect();
}

void RtspServer::handleCloseConnect()
{
    std::lock_guard<std::mutex> lck(mMtx);
    for(int clientfd : mDisConns)
    {
        auto it = mConnMap.find(clientfd);
        assert(it != mConnMap.end());
        delete it->second;
        mConnMap.erase(it);
    }
    mDisConns.clear();
}
