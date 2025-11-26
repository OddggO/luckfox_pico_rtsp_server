#include "H264MediaSource.h"
#include "Log.h"

static inline bool startCode3(uint8_t* buf);
static inline bool startCode4(uint8_t* buf);

H264MediaSource* H264MediaSource::createNew(UsageEnvironment* env, std::string source)
{
    return new H264MediaSource(env, source);
}

H264MediaSource::H264MediaSource(UsageEnvironment* env, std::string source): MediaSource(env)
{
    mSourceName = source;
    mFile = fopen(mSourceName.data(), "rb"); // r: 只读，b: 二进制打开
    if (!mFile) {
        LOGE("open file %s error", mSourceName.c_str());
        return;
    }
    setFps(25);
    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
    {
        mEnv->threadPool()->addTask(mTask);
    }
}

H264MediaSource::~H264MediaSource()
{
    fclose(mFile);
}

void H264MediaSource::handleTask()
{
    std::lock_guard<std::mutex> lck(mMtx);
    if (mInputFrameQ.empty()) {
        LOGI("no input frame available");
        return;
    }
    MediaFrame* frame = mInputFrameQ.front();
    // mInputFrameQ.pop(); // 需要退出循环再pop，否则里面代码可能直接return
    while (true) // 一直读，直到读到一帧数据，或者完全没有一帧数据
    {
        int startCode = 0;
        frame->mSize = getFrameFromH264File(frame->temp, FRAME_MAX_SIZE, startCode);
        if (frame->mSize < 0) {
            LOGE("read frame failed");
            return;
        }
        frame->mBuf = frame->temp + startCode;
        frame->mSize -= startCode;
        // int naluType = frame->mBuf[0] & 0x1f;
        uint8_t naluType = frame->mBuf[0] & 0x1f;
        // LOGI("Read frame size = %d, naluType = 0x%02x, nalyType=%d", frame->mSize, naluType, naluType);
        if (naluType == 0x09) { // 0x09表示一帧的开始，需要
            continue;
        } else if (naluType == 0x07 || naluType == 0x08) {
            break;
        } else {
            break;
        }
    }
    mInputFrameQ.pop();
    mOutputFrameQ.push(frame);

}

static inline bool startCode3(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] ==0 && buf[2] == 1)
        return true;
    else
        return false;
}

static inline bool startCode4(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] ==0 && buf[2] == 0 && buf[3] == 1)
        return true;
    else
        return false;
}

static uint8_t* findNextStartCode(uint8_t* buf, int size)
{
    if (size < 3)
        return nullptr;
    uint8_t* nextStartCode = buf;
    while (nextStartCode && (nextStartCode - buf) < size - 3)
    {
        if(startCode3(nextStartCode) || startCode4(nextStartCode))
            return nextStartCode;
        nextStartCode += 1;
    }
    if (startCode3(nextStartCode)) {
        return nextStartCode;
    }
    return nullptr;
}

// static uint8_t* findNextStartCode(uint8_t* buf, int len)
// {
//     int i;

//     if (len < 3)
//         return NULL;

//     for (i = 0; i < len - 3; ++i)
//     {
//         if (startCode3(buf) || startCode4(buf))
//             return buf;

//         ++buf;
//     }

//     if (startCode3(buf))
//         return buf;

//     return NULL;
// }

int H264MediaSource::getFrameFromH264File(uint8_t* frame, int size, int& startCode)
{
    if (!mFile)
    {
        return -1;
    }
    int readSize = fread(frame, 1, size, mFile);
    if (startCode3(frame)) {
        startCode = 3;
    } else if (startCode4(frame)) {
        startCode = 4;
    } else {
        LOGE("Read %s error, no startCode found\n", mSourceName.c_str());
        startCode = -1;
        fseek(mFile, 0, SEEK_SET); // 回到文件起始位置
        return -1;
    }
    uint8_t* nextStartCode = findNextStartCode(frame + 3, readSize - 3);
    int frameSize = 0;
    if (!nextStartCode)
    {
        fseek(mFile, 0, SEEK_SET); // 回到文件起始位置
        frameSize = readSize;
        LOGE("Read %s error, no nextStartCode found, readSize = %d\n", mSourceName.c_str(), readSize);
    } else
    {
        frameSize = (nextStartCode - frame);
        fseek(mFile, frameSize - readSize, SEEK_CUR);
    }
    return frameSize;
}
