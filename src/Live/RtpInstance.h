#pragma once
#include "Rtp.h"
#include "IPv4Address.h"
#include "SocketOps.h"

class RtpInstance
{
public:
    enum RtpType
    {
        RTP_OVER_UDP, 
        RTP_OVER_TCP
    };
    static RtpInstance* createNewOverUdp(int localSockfd, int localPort, const std::string destIp, uint16_t destPort)
    {
        return new RtpInstance(localSockfd, localPort, destIp, destPort);
    }
        
    RtpInstance(int localSockfd, int localPort, const std::string destIp, uint16_t destPort):
        mRtpType(RTP_OVER_UDP), mSockfd(localSockfd), mLocalPort(localPort), mDestAddr(destIp, destPort), 
        mIsAlive(false), mSessionId(0), mRtpChannel(0)
    {}
    ~RtpInstance()
    {
        // 关闭mSockfd
        sockets::close(mSockfd);
    }

    int send(RtpPacket* rtpPackte)
    {
        if (mRtpType == RTP_OVER_UDP) {
            return sendOverUdp(rtpPackte->mBuf4, rtpPackte->mSize);
        } else if (mRtpType == RTP_OVER_TCP) {
            rtpPackte->mBuf[0] = '$';
            rtpPackte->mBuf[1] = (uint8_t)mRtpChannel;
            // 16位网络字节序
            rtpPackte->mBuf[2] = (uint8_t)((rtpPackte->mSize && 0xff00) >> 8);
            rtpPackte->mBuf[3] = (uint8_t)(rtpPackte->mSize && 0x00ff);
            return sendOverTcp(rtpPackte->mBuf, 4 + rtpPackte->mSize);
        } 
        return -1;
    }

    bool alive() const {return mIsAlive;}
    int setAlive(bool isAlive) {mIsAlive = isAlive; return 0;}
    void setSessionId(uint8_t sessionId) {mSessionId = sessionId;}
    uint8_t sessionId() const {return mSessionId;}
    uint16_t getLocalPort() const {return mLocalPort;}
    uint16_t getDestPort() {return mDestAddr.getPort();}
    void setRtpChannel(uint8_t rtpChannel) {mRtpChannel = rtpChannel;}
    uint8_t rtpChannel() const {return mRtpChannel;}
private:
    int sendOverUdp(void* buf, int size)
    {
        return sockets::sendto(mSockfd, buf, size, mDestAddr.getAddr());
    }
    int sendOverTcp(void* buf, int size)
    {
        return sockets::write(mSockfd, buf, size);
    }

private:
    RtpType mRtpType;
    int mSockfd;
    uint16_t mLocalPort; // for udp
    IPv4Address mDestAddr; // for udp
    bool mIsAlive;
    uint16_t mSessionId;
    uint8_t mRtpChannel;
};

class RtcpInstance
{
public:
    static RtcpInstance* createNew(int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort)
    {
        return new RtcpInstance(localSockfd, localPort, destIp, destPort);
    }
    RtcpInstance(int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort): 
                    mLocalSockfd(localSockfd), mLocalPort(localPort), mDestAddr(destIp, destPort), 
                    mIsAlive(false), mSessionId(0)
    {}
    ~RtcpInstance()
    {
        sockets::close(mLocalSockfd);
    }

    int send(void* buf, int size)
    {
        return sockets::sendto(mLocalSockfd, buf, size, mDestAddr.getAddr());
    }

    int recv(void* buf, int size, IPv4Address* addr)
    {
        // TODO
        return 0;
    }

    bool alive() const {return mIsAlive;}
    int setAlive(bool isAlive) {mIsAlive = isAlive; return 0;}
    void setSessionId(uint8_t sessionId) {mSessionId = sessionId;}
    uint8_t sessionId() const {return mSessionId;}
    uint16_t getLocalPort() const {return mLocalPort;}
    uint16_t getDestPort() {return mDestAddr.getPort();}
private:
    int mLocalSockfd;
    uint16_t mLocalPort;
    IPv4Address mDestAddr;
    bool mIsAlive;
    uint16_t mSessionId;
};
