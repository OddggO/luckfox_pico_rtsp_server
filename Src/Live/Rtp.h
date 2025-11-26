#pragma once
#include <stdint.h>
#define RTP_VERSION 2 // ？？
#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97
#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400

struct RtpHeader 
{
    // 在变量名后面加":数字"的语法叫"C位域"，对于常见的x86/GCC/Clang编译器（小端序），位域定义顺序一般是从低位到高位依次分配
    // 因此第一个字节会先定义csrcLen最后定义version
    // byte 0
    uint8_t csrcLen:4;      //CSRC计数器，占4位，指示CSRC 标识符的个数。
    uint8_t extension:1;    //占1位，如果X=1，则在RTP报头后跟有一个扩展报头。
    uint8_t padding:1;      // 填充标志，占1位，如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分。
    uint8_t version:2;      // RTP协议的版本号，占2位，当前协议版本号为2。

    // byte 1
    uint8_t payloadType:7;      //有效载荷类型，占7位，用于说明RTP报文中有效载荷的类型，如GSM音频、JPEM图像等。
    uint8_t marker:1;       //标记，占1位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始。

    // byte 2, 3
    uint16_t seq;           //占16位，用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1。
                            //接收者通过序列号来检测报文丢失情况，重新排序报文，恢复数据。

    // byte 4 - 7
    uint32_t timestamp;     //占32位，时戳反映了该RTP报文的第一个八位组的采样时刻。接收者使用时戳来计算延迟和延迟抖动，并进行同步控制。

    // byte 8 - 11
    uint32_t ssrc;          //占32位，用于标识同步信源。该标识符是随机选择的，参加同一视频会议的两个同步信源不能有相同的SSRC。

    // data
    uint8_t payload[0];
};

struct RtcpHeader
{
    // byte 0
    uint8_t rc: 5;
    uint8_t padding: 1;
    uint8_t version: 2;

    // byte 1
    uint8_t packetType;

    // bute 2, 3
    uint16_t length;
};

class RtpPacket
{
public:
    RtpPacket();
    ~RtpPacket();
public:
    uint8_t* mBuf; // 4 + rtpHeader + rtpBody;
    uint8_t* mBuf4; // rtpHeader + rtpBody;
    RtpHeader* const mRtpHeader;
    int mSize; // rtpHeader + rtpBody;
};

void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtpHeader);
void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcpHeader);
