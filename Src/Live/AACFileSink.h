#pragma once
#include "Sink.h"
#include "Rtp.h"

class AACFileSink: public Sink
{
public:
    static AACFileSink* createNew(UsageEnvironment* env, MediaSource* mediaSource);
    AACFileSink(UsageEnvironment* env, MediaSource* mediaSource);
    virtual ~AACFileSink();
    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();
private:
    virtual void sendFrame(MediaFrame* frame);
private:
    RtpPacket mRtpPacket;
    uint32_t mSampleRate;   // 采样频率
    uint32_t mChannels;     // 通道数 TODO 声道？
    int mFps;
};



