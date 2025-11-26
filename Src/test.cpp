#include <iostream>
#include "Log.h"
#include "Version.h"
#include "Scheduler/Scheduler.h"
#include "Scheduler/ThreadPool.h"
#include "Scheduler/UsageEnvironment.h"
#include "Live/RtspServer.h"
#include "Live/MediaSessionManager.h"
#include "Live/MediaSession.h"
#include "Live/MediaSession.h"
#include "Live/H264MediaSource.h"
#include "Live/AACMediaSource.h"
#include "Live/Sink.h"
#include "Live/H264FileSink.h"
#include "Live/AACFileSink.h"

int main()
{
    // q: srand(time(NULL));的作用是什么？a: 用当前时间初始化随机数生成器的种子，确保每次运行程序时生成不同的随机数序列。生成SessionId和多播地址需要rand()函数产生随机数
    srand(time(NULL)); // 时间初始化    
    LOGI("program begin");
    Scheduler* scheduler = Scheduler::createNew(Scheduler::PollerType::SelectPoller);
    ThreadPool* threapool = ThreadPool::createNew(1);
    UsageEnvironment* env = UsageEnvironment::createNew(scheduler, threapool);
    MediaSessionManager* sessionMgr = MediaSessionManager::createNew();
    RtspServer* rtspServer = RtspServer::createNew(env, sessionMgr, "0.0.0.0", 8554);

    MediaSession* session = MediaSession::createNew("test");
    
    MediaSource* h264Source = H264MediaSource::createNew(env, "data/daliu.h264");
    Sink* h264Sink = H264FileSink::createNew(env, h264Source);
    session->addSink(MediaSession::Track0, h264Sink);

    MediaSource* aacSource = AACMediaSource::createNew(env, "data/daliu.aac");
    Sink* aacSink = AACFileSink::createNew(env, aacSource);
    session->addSink(MediaSession::Track1, aacSink);
    // session->startMulticast(); // 开启多播
    sessionMgr->addSession(session);

    rtspServer->start();
    // scheduler->loop();
    env->scheduler()->loop();

    return 0;
}