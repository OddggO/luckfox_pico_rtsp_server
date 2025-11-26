#include "AACMediaSource.h"
#include <string.h>
#include "Log.h"

AACMediaSource* AACMediaSource::createNew(UsageEnvironment* env, const std::string& source)
{
    return new AACMediaSource(env, source);
}

AACMediaSource::AACMediaSource(UsageEnvironment* env, const std::string& source): MediaSource(env)
{
    mSourceName = source;
    mFile = fopen(mSourceName.c_str(), "rb");
    setFps(43);
    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
    {
        mEnv->threadPool()->addTask(mTask);
    }
}

AACMediaSource::~AACMediaSource()
{
    fclose(mFile);
}

void AACMediaSource::handleTask()
{
    std::lock_guard<std::mutex> lck(mMtx);
    if (mInputFrameQ.empty()) {
        return;
    }
    MediaFrame* frame = mInputFrameQ.front();
    frame->mSize = getFrameFromAACFile(frame->temp, FRAME_MAX_SIZE);
    if (frame->mSize < 0) {
        return;
    }
    frame->mBuf = frame->temp;
    mInputFrameQ.pop();
    mOutputFrameQ.push(frame);
}

bool AACMediaSource::parseAdtsHeader(uint8_t* in, struct AdtsHeader* res)
{
    // 注意：in是读取的本地文件，所以是主机字节序，小端，低位在低字节
    memset(res, 0, sizeof(struct AdtsHeader));
    if (in[0] == 0xff && ((in[1] & 0xf0) == 0xf0)) {
        res->id = ((unsigned int) in[1] & 0x08) >> 3;
        res->layer = ((unsigned int) in[1] & 0x06) >> 1;
        res->protectionAbsent = (unsigned int) in[1] & 0x01;
        // TODO 确认profile应该是1位还是2位
        res->profile = ((unsigned int) in[2] & 0xc0) >> 6;
        // res->profile = ((unsigned int)in[2] & 0x80) >> 7;
        res->samplingFreqIndex = ((unsigned int) in[2] & 0x3c) >> 2;
        res->privateBit = ((unsigned int) in[2] & 0x02) >> 1;
        res->channelCfg = ((((unsigned int) in[2] & 0x01) << 2) | (((unsigned int) in[3] & 0xc0) >> 6));
        res->originalCopy = ((unsigned int) in[3] & 0x20) >> 5;
        res->home = ((unsigned int) in[3] & 0x10) >> 4;
        res->copyrightIdentificationBit = ((unsigned int) in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = (unsigned int) in[3] & 0x04 >> 2;
        res->aacFrameLength = (((((unsigned int) in[3]) & 0x03) << 11) |
                                (((unsigned int)in[4] & 0xFF) << 3) |
                                    ((unsigned int)in[5] & 0xE0) >> 5) ;
        res->adtsBufferFullness = (((unsigned int) in[5] & 0x1f) << 6 |
                                        ((unsigned int) in[6] & 0xfc) >> 2);
        res->numberOfRawDataBlockInFrame = ((unsigned int) in[6] & 0x03);
        return true;
    }
    LOGE("failed to parse adts header");
    return false;
}

int AACMediaSource::getFrameFromAACFile(uint8_t* buf, int size)
{
    if (!mFile) {
        return -1;
    }
    uint8_t tmpBuf[7];
    int ret = fread(tmpBuf, 1, 7, mFile);
    if (ret <= 0) {
        fseek(mFile, 0, SEEK_SET);
        ret = fread(tmpBuf, 1, 7, mFile);
        if (ret <= 0) {
            return -1;
        }
    }
    if (!parseAdtsHeader(tmpBuf, &mAdtsHeader)) {
        return -1;
    }
    if (mAdtsHeader.aacFrameLength > size) {
        return -1;
    }
    memcpy(buf, tmpBuf, 7); // 复制adts头
    ret = fread(buf + 7, 1, mAdtsHeader.aacFrameLength - 7, mFile);
    if (ret < 0) {
        LOGE("read error");
        return -1;
    }
    return mAdtsHeader.aacFrameLength;
}
