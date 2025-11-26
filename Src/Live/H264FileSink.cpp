#include "H264FileSink.h"
#include <string.h>
#include "Log.h"

H264FileSink* H264FileSink::createNew(UsageEnvironment* env, MediaSource* mediaSource)
{
    if (!mediaSource) {
        LOGE("H264FileSink::createNew failed, mediaSource is nullptr");
        return nullptr;
    }
    return new H264FileSink(env, mediaSource);
}

H264FileSink::H264FileSink(UsageEnvironment* env, MediaSource* mediaSource): Sink(env, mediaSource, RTP_PAYLOAD_TYPE_H264), 
                            mClockRate(90000), mFps(mediaSource->getFps())
{
    LOGI("H264FileSink()");
    runEvery(1000 / mFps); // fps = 每秒处理的帧数，因此除fps就是每帧需要的时间，*1000应该是为了转化秒为毫秒
}

H264FileSink::~H264FileSink()
{
    LOGI("~H264FileSink()");
}

std::string H264FileSink::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP %d", port, mPayloadType);
    return std::string(buf);
}

std::string H264FileSink::getAttribute()
{
    char buf[100];
    int len = sprintf(buf, "a=rtpmap:%d H264/%d\r\n", mPayloadType, mClockRate);
    sprintf(buf + len, "a=framerate:%d", mFps); // 这里最后不要加\r\n，MediaSessio生成sdp时会添加
    return std::string(buf);
}

void H264FileSink::sendFrame(MediaFrame* frame)
{
    uint8_t naluType = frame->mBuf[0];
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeader; // mRtpPacket的构造函数已经为rtpHeader及其payload开辟了足够的空间
    // int ret, sendBytes = 0;
    if (frame->mSize <= RTP_MAX_PKT_SIZE) {
        memcpy(rtpHeader->payload, frame->mBuf, frame->mSize);
        mRtpPacket.mSize = RTP_HEADER_SIZE + frame->mSize;
        // mMarker = 1; // 传输h264时，单NALU帧为视频帧仅有也是最后一个包，需要设置marker为1（AAC不需要，一直保持为0即可）
        sendRtpPacket(&mRtpPacket);
        mSeq++;

        if ((naluType & 0x1f) == 7 || (naluType == 0x1f) == 8) {
            // SPS SSP 不需要时间戳
            return;
        }
    } 
    else {
        // if (naluType == 0x07 || naluType == 0x08) {
        //     // SPS SSP 不进行分片传输
        //     LOGE("SPS/PPS size %d exceed RTP_MAX_PKT_SIZE %d, drop it\n", frame->mSize, RTP_MAX_PKT_SIZE);
        //     return;
        // }
        /**
         * h264 nalu分片打包
         * 需要额外使用两个字节标识，第一个字节如：
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         * NRI和naluType一致（即nalu的第一帧的1 2bit）
         * Type固定为28
         * 
         * 第二个字节如：
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         * S = 1 表示第一帧，E = 1 表示最后一帧
         * 
         * 注意，对于h264，如果是一个nalu（帧）的最后包，需要设置mark=1，否则为0
         */
        // int pktNum = mRtpPacket.mSize / RTP_MAX_PKT_SIZE;
        // int remainPktSize = mRtpPacket.mSize % RTP_MAX_PKT_SIZE;
        int pktNum = frame->mSize / RTP_MAX_PKT_SIZE;
        int remainPktSize = frame->mSize % RTP_MAX_PKT_SIZE;
        int i, pos = 1;

        for (i = 0; i < pktNum; ++i)
        {
            rtpHeader->payload[0] = (naluType & 0x60) | 28; // TODO: 是否应该是0xE0
            rtpHeader->payload[1] = naluType & 0x1f;
            if (i == 0) {
                rtpHeader->payload[1] |= 0x80;
                // mMarker = 0; // 传输h264时，一帧图像的非最后一个包需要设置marker为0
            }
            else if (remainPktSize == 0 && i == pktNum - 1) {
                rtpHeader->payload[1] |= 0x40;
                // mMarker = 1; // 传输h264时，一帧图像的最后一个包需要设置marker为1
            }
            memcpy(rtpHeader->payload + 2, frame->mBuf + pos, RTP_MAX_PKT_SIZE);
            mRtpPacket.mSize = RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 2;
            sendRtpPacket(&mRtpPacket);
            // sendBytes += ret;
            mSeq++;
            pos += RTP_MAX_PKT_SIZE;
        }
        
        if (remainPktSize > 0)
        {
            rtpHeader->payload[0] = (naluType & 0x60) | 28; // TODO: 是否应该是0xE0
            rtpHeader->payload[1] = naluType & 0x1F; 
            rtpHeader->payload[1] |= 0x40;
            // memcpy(rtpHeader->payload + 2, frame + pos, remainPktSize + 2);
            memcpy(rtpHeader->payload + 2, frame->mBuf + pos, remainPktSize); // TODO: 这里应该不用+2
            mRtpPacket.mSize = RTP_HEADER_SIZE + remainPktSize + 2;
            // mMarker = 1; 
            sendRtpPacket(&mRtpPacket);
            // sendBytes += ret;
            mSeq++;
            pos += remainPktSize;
        }
    }

    mTimestamp += mClockRate / mFps;
}
