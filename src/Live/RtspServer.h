#pragma once
#include "UsageEnvironment.h"
#include "SocketOps.h"
#include "Event.h"
// #include "RtspConnection.h"
#include <map>
#include <vector>
#include "MediaSessionManager.h"
#include <thread>
#include "IPv4Address.h"

class RtspConnection;
class RtspServer
{
public:
    static RtspServer* createNew(UsageEnvironment* env, MediaSessionManager* sessionManager, const std::string& ip, uint16_t port);
    RtspServer(UsageEnvironment* env, MediaSessionManager* sessionManager, const std::string& ip, uint16_t port);
    ~RtspServer();

    void start();
    UsageEnvironment* env() const;
    MediaSessionManager* mediaSessionManager();
private:
    static void acceptCallback(void* arg);
    void handleAccept();
    /**
     * 为什么要分disConnect和closeConnect？
     * 这是因为disConnect需要由RtspConnection执行，而RtspConnection对象执行回调删除自己，
     * 程序难以控制，因此在closeConnect回调删除，并且交由调度类Scheduler执行
     */
    static void disConnectCallback(void* arg, int clientfd);
    void handleDisConnect(int clientfd);
    static void closeConnectCallback(void* arg);
    void handleCloseConnect();
private:
    int mFd;
    UsageEnvironment* mEnv;
    MediaSessionManager* mSessionManager;
    IPv4Address mIpv4Address;
    bool mListen;
    
    IOEvent* mAcceptEvent;
    TriggerEvent* mCloseConnectEvent;

    std::map<int, RtspConnection*> mConnMap;
    std::vector<int> mDisConns;
    std::mutex mMtx;
};
