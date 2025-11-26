#include "Sink.h"
#include "Rtp.h"
#include <arpa/inet.h>
#include "Log.h"

Sink::Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType): 
            mEnv(env), mMediaSource(mediaSource), mPayloadType(payloadType), 
            mCsrcLen(0),
            mExtension(0),
            mPadding(0),
            mVersion(RTP_VERSION),
            mMarker(0),
            mSeq(0),
            mSSRC(rand()),
            mTimestamp(0),
            mTimerId(0),
            mSessionSendPacketCb(nullptr),
            mArg1(nullptr),
            mArg2(nullptr)
{
    LOGI("Sink()");
    mTimerEvent = TimerEvent::createNew(timeoutCb, this);
}

Sink::~Sink()
{
    if (mTimerEvent) {
        mEnv->scheduler()->removeTimerEvent(mTimerId);
        delete mTimerEvent;
    }
    if (mMediaSource)
        delete mMediaSource; // mMediaSource为什么由Sink删除？
}

void Sink::stopTimerEvent()
{
    mTimerEvent->stop();
    // mEnv->scheduler()->removeTimerEvent(mTimerId);
}

void Sink::setSessionSendPacketCb(SessionSendPacketCallback cb, void* arg1, void* arg2)
{
    mSessionSendPacketCb = cb;
    mArg1 = arg1;
    mArg2 = arg2;
}

void Sink::timeoutCb(void* arg)
{
    Sink* sink = (Sink*)arg;
    sink->handleTimeout();
}

void Sink::handleTimeout()
{
    MediaFrame* frame = mMediaSource->getFromOutputQueue();
    if (!frame) {
        return;
    }
    sendFrame(frame); // 由具体的子类实现，如发送h264帧由H264Sink实现
    mMediaSource->putFrameToInputQueue(frame);
}

void Sink::sendRtpPacket(RtpPacket* rtpPacket)
{
    RtpHeader* rtpHeader = rtpPacket->mRtpHeader;
    rtpHeader->csrcLen = mCsrcLen;
    rtpHeader->extension = mExtension;
    rtpHeader->padding = mPadding;
    rtpHeader->version = mVersion;
    rtpHeader->payloadType = mPayloadType;
    rtpHeader->marker = mMarker;
    rtpHeader->seq = htons(mSeq);
    rtpHeader->timestamp = htonl(mTimestamp);
    rtpHeader->ssrc = htonl(mSSRC);

    if (mSessionSendPacketCb) {
        mSessionSendPacketCb(mArg1, mArg2, rtpPacket, RTPPACKET);
    }
}

void Sink::runEvery(int interval)
{
    mTimerId = mEnv->scheduler()->addTimerEventRunEvery(mTimerEvent, interval);
}
