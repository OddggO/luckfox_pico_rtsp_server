#include "MediaSession.h"
#include "Log.h"
#include <assert.h>
#include <string.h>

MediaSession* MediaSession::createNew(const std::string name)
{
    return new MediaSession(name);
}
MediaSession::MediaSession(const std::string name): mSessionName(name), mIsMulticast(false)
{
    LOGI("MediaSession(), mSessionName=%s", mSessionName.c_str());
    // mTrack[0].mTrackId = TrackNone; // 已经设置了默认初始化，不需要再显式初始化它
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        mTrack[i].mTrackId = static_cast<MediaSession::TrackId>(i); // 0 -> Track0, 1 -> Track1 (如果存在)
        mTrack[i].mAlive = false;
        mTrack[i].mSink = nullptr;
        // mTrack[i].mRtpInstances 默认构造为空 list，不需要额外处理
    }
    
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        mMulticastRtpInstances[i] = nullptr;
        mMulticastRtcpInstances[i] = nullptr;
    }
}

MediaSession::~MediaSession()
{
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (mMulticastRtpInstances[i] != nullptr) {
            delete mMulticastRtpInstances[i];
            mMulticastRtpInstances[i] = nullptr;
        }
        if (mMulticastRtcpInstances[i] != nullptr) {
            delete mMulticastRtcpInstances[i];
            mMulticastRtcpInstances[i] = nullptr;
        }
        if (mTrack[i].mAlive) {
            Sink* sink = mTrack[i].mSink;
            delete sink;
            mTrack[i].mSink = nullptr;
        }
    }
}

std::string MediaSession::name()
{
    return mSessionName;
}

std::string MediaSession::generateSdpDescription()
{
    if (!mSdp.empty())
        return mSdp;
    std::string ip = "0.0.0.0";
    char buf[2048] = {0};
    int len = snprintf(buf, sizeof(buf), // snprintf...
    "v=0\r\n"
    "o=- 9%ld 1 IN IP4 %s\r\n"
    "t=0 0\r\n"
    "a=control:*\r\n"
    "a=type:broadcast\r\n",
    (long)time(nullptr), ip.c_str());

    if (mIsMulticast) {
        len += snprintf(buf + len, sizeof(buf) - len, 
                        "a=rtcp-unicast: reflection\r\n");
    }

    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (mTrack[i].mAlive == false) {
            continue;
        }
        // if (i != 1) {
        //     continue;
        // }
        /*
        * 此处，如果单播，服务器并不能提前知道要是用端口号，而是需要等到客户端发送SETUP请求才能知道
        * 因此单播使用0表示端口待定
        * 而在组播时，服务端会提前分配好一组多播IP和端口，并向这个端口发送。因此，多播可以知道端口号
        */
        uint16_t port = 0;
        if (mIsMulticast) {
            port = getMulticastDestRtpPort(static_cast<TrackId>(i));
        }
        len += snprintf(buf + len, sizeof(buf) - len, "%s\r\n", mTrack[i].mSink->getMediaDescription(port).c_str());
        
        if (mIsMulticast) {
            len += snprintf(buf + len, sizeof(buf) - len, "c=IN 1P4 %s/255\r\n", getMulticastDestAddr().c_str());
        } else {
            len += snprintf(buf + len, sizeof(buf) - len, "c=IN IP4 0.0.0.0\r\n");
        }
        len += snprintf(buf + len, sizeof(buf) - len, "%s\r\n", mTrack[i].mSink->getAttribute().c_str());
        len += snprintf(buf + len, sizeof(buf) - len, "a=control:track%d\r\n", (int)mTrack[i].mTrackId);
    }
    LOGI("buf: %s", buf);
    // LOGI("mSdp: %s, size: %d", mSdp.c_str());
    // LOGI("mSdp: %s", mSdp.data());
    LOGI("mSdp size: %d", (int)mSdp.size());
    mSdp = buf;
    return mSdp;
}

bool MediaSession::addSink(TrackId trackId, Sink* sink)
{
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    track->mSink = sink;
    track->mAlive = true;
    track->mSink->setSessionSendPacketCb(MediaSession::SessionSendPacketCb, this, track);
    return true;
}

bool MediaSession::removeSink(Sink* sink)
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (sink == mTrack[i].mSink) {
            mTrack[i].mSink = nullptr;
            return true;
        }
    }
    return false;
}

bool MediaSession::addRtpInstance(TrackId trackId, RtpInstance* rtpInstance)
{
    Track* track = getTrack(trackId);
    if (!track || track->mAlive == false) {
        return false;
    }
    track->mRtpInstances.push_back(rtpInstance);
    return true;
}

bool MediaSession::removeRtpInstance(RtpInstance* rtpInstance)
{
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) 
    {
        Track* track = &mTrack[i];
        auto it = track->mRtpInstances.begin();
        while (it != track->mRtpInstances.end())
        {
            if (*it == rtpInstance) {
                track->mRtpInstances.erase(it);
                return true;
            }
            it++;
        }
        // std::find书写更简便
    }
    return false;
}

void MediaSession::SessionSendPacketCb(void* arg1, void* arg2, RtpPacket* rtpPacket, Sink::PacketType packetType)
{
    MediaSession* session = (MediaSession*)arg1;
    Track* track = (Track*)arg2;
    session->sendPacket(track, rtpPacket);
}

void MediaSession::sendPacket(Track* track, RtpPacket* rtpPacket)
{
    for (auto& rtpInstance : track->mRtpInstances)
    {
        if (rtpInstance->alive()) {
            int ret = rtpInstance->send(rtpPacket);
            LOGI("send %d beytes, %s, (errno=%d)\n", ret, strerror(errno), errno);
        }
    }
}

void MediaSession::startMulticast()
{
    // 随机生成多播地址
    struct sockaddr_in addr = {0};
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand()) % range);
    mMulticastAddr = inet_ntoa(addr.sin_addr);
    int rtpSockfd1, rtcpSockfd1;
    int rtpSockfd2, rtcpSockfd2;
    uint16_t rtpPort1, rtcpPort1;
    uint16_t rtpPort2, rtcpPort2;
    bool ret;

    rtpSockfd1 = sockets::createUdpSocket();
    assert(rtpSockfd1 > 0);

    rtpSockfd2 = sockets::createUdpSocket();
    assert(rtpSockfd2 > 0);

    rtcpSockfd1 = sockets::createUdpSocket();
    assert(rtcpSockfd1 > 0);

    rtcpSockfd2 = sockets::createUdpSocket();
    assert(rtcpSockfd2 > 0);

    // 绑定端口，发送端的套接字并不重要，不在sdp中显示，
    // 因此设置为0，让系统自动分配
    sockets::bind(rtpSockfd1, sockets::getLocalIp(), 0);
    sockets::bind(rtpSockfd1, sockets::getLocalIp(), 0);
    sockets::bind(rtcpSockfd1, sockets::getLocalIp(), 0);
    sockets::bind(rtcpSockfd2, sockets::getLocalIp(), 0);

    // 以下端口是多播目标地址的端口，不是套接字需要绑定的端口
    uint16_t port;
    // RTP端口必须是偶数，rtcp则是rtp+1
    port = rand() & 0xfffc;  // 1111 1111 1111 1100，最后两位清零，这样port是4的倍数
    if (port < 10000)
        port += 10000; // 防止使用系统预留端口
    rtpPort1 = port;
    rtcpPort1 = port+1;
    rtpPort2 = rtcpPort1+1;
    rtcpPort2 = rtpPort2+1;

    mMulticastRtpInstances[0] = RtpInstance::createNewOverUdp(rtpSockfd1, 0, mMulticastAddr, rtpPort1); // UDP
    mMulticastRtpInstances[1] = RtpInstance::createNewOverUdp(rtpSockfd2, 0, mMulticastAddr, rtpPort2); // UDP
    mMulticastRtcpInstances[0] = RtcpInstance::createNew(rtcpSockfd1, 0, mMulticastAddr, rtcpPort1);
    mMulticastRtcpInstances[1] = RtcpInstance::createNew(rtcpSockfd2, 0, mMulticastAddr, rtcpPort2);
    this->addRtpInstance(Track0, mMulticastRtpInstances[0]);
    this->addRtpInstance(Track1, mMulticastRtpInstances[1]);
    mMulticastRtpInstances[0]->setAlive(true);
    mMulticastRtpInstances[1]->setAlive(true);

    mIsMulticast = true;
}

bool MediaSession::isMulticast()
{
    return mIsMulticast;
}

std::string MediaSession::getMulticastDestAddr() const
{
    return mMulticastAddr;
}

uint16_t MediaSession::getMulticastDestRtpPort(TrackId trackId)
{
    if (trackId == TrackNone)
        return -1;
    if (trackId != Track0 && trackId != Track1)
        return -1;
    if (!mMulticastRtpInstances[trackId])
        return -1;
    return mMulticastRtpInstances[trackId]->getDestPort();
}

MediaSession::Track* MediaSession::getTrack(TrackId trackId)
{
    if (trackId == TrackNone)
        return nullptr;
    if (trackId != Track0 && trackId != Track1)
        return nullptr;
    return &mTrack[trackId];
}
