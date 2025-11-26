#include "Rtp.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

RtpPacket::RtpPacket(): mBuf((uint8_t*)malloc(4 + RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100)), 
                        mBuf4(mBuf + 4), mRtpHeader((RtpHeader*)mBuf4), mSize(0)
{   
}

RtpPacket::~RtpPacket()
{
    free(mBuf);
    mBuf = NULL;
    mBuf4 = NULL;
}

// void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtpHeader)
// {
//     // 注意：buf是网络传输来的，因此是网络字节序，大端，高位在低字节
//     memset(rtpHeader, 0, sizeof(*rtpHeader));
//     // byte 0
//     // 字节序仅仅影响字节间的顺序，对于字节内的bit的顺序不影响
//     // 因此，byte 0的解析不收网络字节序的影响，还是按照编译器位域顺序解析
//     rtpHeader->version = (buf[0] & 0xc0) >> 6;
//     rtpHeader->padding = (buf[0] & 0x20) >> 5;
//     rtpHeader->extension = (buf[0] & 0x10) >> 4;
//     rtpHeader->csrcLen = (buf[0] & 0x0f);
//     // byte 1
//     rtpHeader->marker = (buf[1] & 0x80) >> 7;
//     rtpHeader->payloadType = (buf[1] & 0x7f);
//     // byte 2, 3
//     // buf是网络io接收的rtp协议流，因此是网络字节序（大端），所以buf[2]是高位，转换到主机字节序（小端）时，
//     // 需要将低位放在低字节
//     // rtpHeader->seq = (buf[2] & 0xff) | ((buf[3] & 0xff) << 8);
//     rtpHeader->seq = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff); 
//     // 也可以这样写
//     // rtpHeader->seq = ntohs(*(uint16_t*)(buf + 2)); // 网络字节序 -> 主机字节序
//     // byte 4 - 7
//     rtpHeader->timestamp = ((buf[4] * 0xff) << 24) | ((buf[5] * 0xff) << 16) | 
//                            ((buf[6] * 0xff) << 8) | (buf[7] * 0xff);
//     // rtpHeader->timestamp = ntohl(*(uint32_t*)(buf + 4));
//     // byte 8 - 11
//     // rtpHeader->ssrc = ntohl(*(uint32_t*)(buf + 8));
//     rtpHeader->ssrc = ((buf[8] * 0xff) << 24) | ((buf[9] * 0xff) << 16) | 
//                       ((buf[10] * 0xff) << 8) | (buf[11] * 0xff);
// }

void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtpHeader)
{
    memset(rtpHeader, 0, sizeof(*rtpHeader));

    // byte 0
    rtpHeader->version = (buf[0] & 0xC0) >> 6;
    rtpHeader->padding = (buf[0] & 0x20) >> 5;
    rtpHeader->extension = (buf[0] & 0x10) >> 4;
    rtpHeader->csrcLen = (buf[0] & 0x0F);
    // byte 1
    rtpHeader->marker = (buf[1] & 0x80) >> 7;
    rtpHeader->payloadType = (buf[1] & 0x7F);
    // bytes 2,3
    rtpHeader->seq = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
    // bytes 4-7
    rtpHeader->timestamp = ((buf[4] & 0xFF) << 24) | ((buf[5] & 0xFF) << 16)
        | ((buf[6] & 0xFF) << 8)
        | ((buf[7] & 0xFF));
    // bytes 8-11
    rtpHeader->ssrc = ((buf[8] & 0xFF) << 24) | ((buf[9] & 0xFF) << 16)
        | ((buf[10] & 0xFF) << 8)
        | ((buf[11] & 0xFF));

}


void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcpHeader)
{
    // byte 0
    rtcpHeader->version = (buf[0] & 0xc0) >> 6;
    rtcpHeader->padding = (buf[0] & 0x20) >> 5;
    rtcpHeader->rc = (buf[0] & 0x1f);
    // byte 1
    rtcpHeader->packetType = buf[1];
    // byte 2, 3
    rtcpHeader->length = ((buf[2]) << 8) | (buf[3]);
}
