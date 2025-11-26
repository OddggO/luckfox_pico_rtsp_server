#pragma once
#include "UsageEnvironment.h"
#include "Event.h"
#include <stdint.h>
#include "MediaSource.h"
#include <string>
#include "Rtp.h"

class Sink
{
public:
    enum PacketType
    {
        UNKNOWN = -1,
        RTPPACKET = 0
    };
    Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType);
    virtual ~Sink();

    void stopTimerEvent();
    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    typedef void (*SessionSendPacketCallback)(void* arg1, void* arg2, RtpPacket* packet, PacketType packType);
    // void setSessionSendPacketCb(SessionSendPacketCallback cb, void* arg1, void* arg2, void* packet, PacketType packType);
    void setSessionSendPacketCb(SessionSendPacketCallback cb, void* arg1, void* arg2);
private:
    static void timeoutCb(void* arg);
    void handleTimeout();
protected:
    virtual void sendFrame(MediaFrame* frame) = 0;
    void sendRtpPacket(RtpPacket* rtpPacket);
    // 设置监听
    void runEvery(int interval);
protected:
    UsageEnvironment* mEnv;
    MediaSource* mMediaSource;
    SessionSendPacketCallback mSessionSendPacketCb;
    void* mArg1;
    void* mArg2;

    uint8_t mCsrcLen;
    uint8_t mExtension;
    uint8_t mPadding;
    uint8_t mVersion;
    uint8_t mPayloadType;
    uint8_t mMarker;
    uint16_t mSeq;
    uint32_t mTimestamp;
    uint32_t mSSRC;
private:
    TimerEvent* mTimerEvent;
    Timer::Timer_ID mTimerId;
};
