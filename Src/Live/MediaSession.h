#pragma once
#include "Sink.h"
#include "Rtp.h"
#include "RtpInstance.h"
#include <string>
#include <list>
#define MEDIA_MAX_TRACK_NUM 2

class MediaSession
{
public:
    enum TrackId
    {
        TrackNone = -1, 
        Track0 = 0, 
        Track1 = 1
    };

    static MediaSession* createNew(const std::string name);
    MediaSession(const std::string name);
    ~MediaSession();

    std::string name();
    std::string generateSdpDescription();
    bool addSink(TrackId trackId, Sink* sink); // 添加数据生成者
    bool removeSink(Sink* sink);
    bool addRtpInstance(TrackId trackId, RtpInstance* rtpInstance); // 添加输出消费者
    bool removeRtpInstance(RtpInstance* rtpInstance); 
    void startMulticast();
    bool isMulticast();
    std::string getMulticastDestAddr() const;
    uint16_t getMulticastDestRtpPort(TrackId trackId);
private:
    class Track
    {
    public:
        TrackId mTrackId = TrackNone;
        bool mAlive = false;
        Sink* mSink = NULL;
        std::list<RtpInstance*> mRtpInstances;
    };
    static void SessionSendPacketCb(void* arg1, void* arg2, RtpPacket* rtpPacket, Sink::PacketType packetType);
    void sendPacket(Track* track, RtpPacket* rtpPacket);
    Track* getTrack(TrackId trackId);   // 根据TrackId获取track

private:
    // 这个是服务端定义的Session名字，如"test", "stream0"，它不是文件的名字
    std::string mSessionName; 
    Track mTrack[MEDIA_MAX_TRACK_NUM];
    std::string mSdp;
    bool mIsMulticast;
    std::string mMulticastAddr;
    RtpInstance* mMulticastRtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* mMulticastRtcpInstances[MEDIA_MAX_TRACK_NUM];
};