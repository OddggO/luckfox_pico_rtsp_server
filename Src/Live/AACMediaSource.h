#pragma once
#include "MediaSource.h"

class AACMediaSource: public MediaSource
{
public:
    static AACMediaSource* createNew(UsageEnvironment* env, const std::string& source);
    AACMediaSource(UsageEnvironment* env, const std::string& source);
    virtual ~AACMediaSource();
protected:
    virtual void handleTask();
private:
    struct AdtsHeader
    {
        unsigned int syncword;      // 12bit, 同步字，全1
        unsigned int id;            // 1bit，MPEG标识符，0 for MPEG-4, 1 for MPEG-2
        unsigned int layer;         // 2 bit 总是'00'
        unsigned int protectionAbsent; // 1 bit，0表示有crc，1表示没有crc 
        unsigned int profile;           // 1 bit，表示使用哪个级别的AAC
        unsigned int samplingFreqIndex; // 4 bit, 表示使用的采样率
        unsigned int privateBit;        // 1 bit
        unsigned int channelCfg;        // 3 bit，表示声道数
        unsigned int originalCopy;      // 1 bit
        unsigned int home;               // 1 bit
        
        /*以下参数是可变化的，对于每一帧是不同的*/
        unsigned int copyrightIdentificationBit;    // 1 bit
        unsigned int copyrightIdentificationStart;  // 1 bit
        unsigned int aacFrameLength;                // 13 bit，一个ADTS帧的长度包括ADTS头和AC原始流
        unsigned int adtsBufferFullness;            // 11 bit 0x7ff 说明是码率可变的码流
        unsigned int numberOfRawDataBlockInFrame;   // 2 bit
    };

    bool parseAdtsHeader(uint8_t* in, struct AdtsHeader* res);
    int getFrameFromAACFile(uint8_t* buf, int size);
private:
    FILE* mFile;
    struct AdtsHeader mAdtsHeader;
};

