#pragma once
#include "UsageEnvironment.h"
#include "ThreadPool.h"
#include <stdint.h>
#include <queue>
#include <mutex>
#include <string>

#define FRAME_MAX_SIZE (1024*200) 
#define DEFAULT_FRAME_NUM   4

class MediaFrame
{
public:
    MediaFrame(): temp(new uint8_t[FRAME_MAX_SIZE]), mBuf(nullptr), mSize(0){}
    ~MediaFrame() {delete []temp; temp = nullptr; mBuf = nullptr;}
    uint8_t* temp; // 容器，如果是h264文件的一帧图像，则包含起始码
    uint8_t* mBuf; // 引用容器
    int mSize;
};

class MediaSource
{
public:
    explicit MediaSource(UsageEnvironment* env);
    virtual ~MediaSource();

    MediaFrame* getFromOutputQueue();
    void putFrameToInputQueue(MediaFrame* frame);
    int getFps() const {return mFps;}
    std::string getSourceName() {return mSourceName;}
private:
    static void taskCallback(void* arg);
protected:
    virtual void handleTask() = 0; // 纯虚函数
    void setFps(int fps) {mFps = fps;}
protected:
    UsageEnvironment* mEnv;
    MediaFrame mFrames[DEFAULT_FRAME_NUM];
    std::queue<MediaFrame*> mInputFrameQ; // 输入帧队列，输入和输出是以此类为中心定义的
    std::queue<MediaFrame*> mOutputFrameQ; // 输出帧队列

    std::mutex mMtx;
    ThreadPool::Task mTask;
    int mFps;
    std::string mSourceName; // 要播放的文件或设备名称
};
