#include "MediaSource.h"
#include "Log.h"

MediaSource::MediaSource(UsageEnvironment* env): mEnv(env), mFps(0), mTask(taskCallback, this)
{
    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
    {
        mInputFrameQ.push(&mFrames[i]);
    }
}

MediaSource::~MediaSource()
{
    LOGI("~MediaSource()");
    // delete 一个非new分配的动态内存，是非法的!
    // 这牵扯到堆内存和栈内存的概念
    // 也就是说，这个类的设计有一定权限，它内部的帧数是有限的，仅有4个


    // while (!mInputFrameQ.empty())
    // {
    //     MediaFrame* frame = mInputFrameQ.front();
    //     delete frame;
    //     frame = nullptr;
    //     mInputFrameQ.pop();
    // }
    // while (!mOutputFrameQ.empty())
    // {
    //     MediaFrame* frame = mOutputFrameQ.front();
    //     delete frame;
    //     frame = nullptr;
    //     mOutputFrameQ.pop();
    // }
}

MediaFrame* MediaSource::getFromOutputQueue()
{
    std::lock_guard<std::mutex> lck(mMtx);
    if (mOutputFrameQ.empty())
        return nullptr;
    MediaFrame* frame = mOutputFrameQ.front();
    mOutputFrameQ.pop();
    return frame;
}

void MediaSource::putFrameToInputQueue(MediaFrame* frame)
{
    std::lock_guard<std::mutex> lck(mMtx);
    mInputFrameQ.push(frame);
    // 向线程池添加任务，添加任务后会唤醒一个子线程处理该任务，也就是执行子类的handleTask函数
    mEnv->threadPool()->addTask(mTask);
}

void MediaSource::taskCallback(void* arg)
{
    MediaSource* source = (MediaSource*)arg;
    source->handleTask();
}
