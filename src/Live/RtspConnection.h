#pragma once
#include "TcpConnection.h"
#include "RtspServer.h"
#include "MediaSession.h"

class RtspServer;
class RtspConnection: public TcpConnection
{
public:
    enum Method
    {
        NONE,
        OPTIONS, 
        DESCRIBE,
        SETUP,
        PLAY,
        PAUSE,
        TEARDOWN
    };
    static RtspConnection* createNew(int clientFd, RtspServer* server);
    RtspConnection(int clientFd, RtspServer* server);
    ~RtspConnection();

protected:
    virtual void handleReadBytes();
    // virtual void handleWrite();
    // virtual void handleError();
private:
    bool parseRequest();
    bool parseFirstLine(const char* first, const char* last);
    bool parseCSeqLine(const char* first, const char* last);
    bool parseLastContent(const char* first, const char* last);

    bool parseDescribe(const char* first, const char* last);
    bool parseSetup(const char* first, const char* last);
    bool parsePlay(const char* first, const char* last);
    bool parsePause(const char* first, const char* last);
    bool parseTeardown(const char* first, const char* last);

    int sendMessage();
    int sendMessage(void* buf, int len);

    bool sendResponse();
    bool sendOptions();
    bool sendDescribe();
    bool sendSetup();
    bool sendPlay();
    bool sendPause();
    bool sendTeardown();

    bool createRtpRtcpPortOverUdp(MediaSession::TrackId trackId, std::string destIp, 
                                    uint16_t destRtpPort, uint16_t destRtcpPort);
private:
    RtspServer* mServer;
    Method mMethodType;
    std::string mStreamPrefix; // 数据流名称，拉流服务中默认是track
    std::string mDestIp; // 客户端ip，由系统函数从mClientFd得到
    /**
     * 不同的请求对应的mUrl和mSuffix是不一样的
     * 对于Options，URL是"rtsp://127.0.0.1:8554/test", 因此mSuffix是"test"
     * 对于Setup，URL是"rtsp://127.0.0.1:8554/test/track0"，因此mSuffix是"test/track0"
     * 客户端是从服务器先前发送的Describe的SDP中得知track0的存在，因此在Setup请求中URL添加了track0
     */
    std::string mUrl;
    std::string mSuffix; // 资源名称
    uint32_t mCSeq; // 客户端请求第二行必携带一个CSeq, 服务端需返回相同的值
    int mDestRtpPort;
    int mDestRtcpPort;
    // 服务端在发送SETUP响应时, 生成一个Session值, 此后客户端的PLAY、PAUSE、TEARDOWN请求携带相同的Session表示是同一次会话. 每次会话的Session是唯一的
    int mSessionId; 
    MediaSession::TrackId mTrackId;
    RtpInstance* mRtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* mRtcpInstances[MEDIA_MAX_TRACK_NUM];
    bool mIsRtpOverTcp;
};
