#include "RtspConnection.h"
#include "Log.h"
#include <string.h>
#include "Version.h"
#include "SocketOps.h"
#define SERVER_NAME "MyRtspServer"

static void getDestIp(int fd, std::string& ip)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&addr, &addrlen);
    ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::createNew(int clientFd, RtspServer* server)
{
    return new RtspConnection(clientFd, server);
}

RtspConnection::RtspConnection(int clientFd, RtspServer* server): TcpConnection(clientFd, server->env()), mServer(server), 
                    mStreamPrefix("track"), mSessionId(rand()), mTrackId(MediaSession::TrackNone), mIsRtpOverTcp(false)
{
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) 
    {
        mRtpInstances[i] = NULL;
        mRtcpInstances[i] = NULL;
    }
    getDestIp(mClientFd, mDestIp);
}

RtspConnection::~RtspConnection()
{
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) 
    {
        if (mRtpInstances[i]) {
            MediaSession* session = mServer->mediaSessionManager()->getSession(mSuffix);
            if (session) {
                session->removeRtpInstance(mRtpInstances[i]);
            }
            delete mRtpInstances[i];
        }
        if (mRtcpInstances[i]) {
            delete mRtcpInstances[i];
        }
    }
}

void RtspConnection::handleReadBytes()
{
    /**
     * 每次不同的RTSP协议交互都会调用一次这个函数，而mUrl等信息是不一样的
     */
    std::string receivedMsg = std::string(mInputBuffer.beginRead(), mInputBuffer.beginWrite());
    LOGI("received msg: \n%s", receivedMsg.c_str());
    // 解析RTSP协议
    if (!parseRequest()) {
        LOGI("parseRequest failed");
        handleDisconnect();
        return;
    }
    // 发送RTSP协议
    if (!sendResponse()) {
        LOGE("sendResponse failed");
        handleDisconnect();
        return;
    }
}

bool RtspConnection::parseRequest()
{
    // 1. 解析第一行
    const char* crlf = mInputBuffer.findNextCRLF(); // 底层const，可以修改指针变量的值，但是不允许修改指针指向内容的值
    if (!crlf || !parseFirstLine(mInputBuffer.beginRead(), crlf)) {
        mInputBuffer.retrievalAll();
        return false;
    }
    mInputBuffer.retrievalUntil(crlf + 2);
    // crlf = mInputBuffer.findNextCRLF();
    // // 2. 解析第一行, 获取CSeq
    // if (!parseCSeqLine(mInputBuffer.beginRead(), crlf)) {
    //     mInputBuffer.retrievalAll();
    //     return false;
    // }
    // mInputBuffer.retrievalUntil(crlf + 2);
    crlf = mInputBuffer.findLastCRLF();
    // 3. 解析剩余内容
    if (!parseLastContent(mInputBuffer.beginRead(), crlf)) {
        mInputBuffer.retrievalAll();
        return false;
    }
    mInputBuffer.retrievalAll();
    return true;
}

bool RtspConnection::parseFirstLine(const char* first, const char* last)
{
    char method[16]; 
    char url[512]; 
    char version[10];
    if (sscanf(first, "%s %s %s\r\n", method, url, version) != 3) {
        return false;
    }
    LOGI("method=%s, url=%s, version=%s", method, url, version);
    if (strcasecmp(method, "OPTIONS") == 0) {
        mMethodType = OPTIONS;
    } else if (strcasecmp(method, "DESCRIBE") == 0) {
        mMethodType = DESCRIBE;
    } else if (strcasecmp(method, "SETUP") == 0) {
        mMethodType = SETUP;
    } else if (strcasecmp(method, "PLAY") == 0) {
        mMethodType = PLAY;
    }  else if (strcasecmp(method, "PAUSE") == 0) {
        mMethodType = PAUSE;
    } else if (strcasecmp(method, "TEARDOWN") == 0) {
        mMethodType = TEARDOWN;
    } else {
        return false;
    }

    if (strcasecmp(version, "RTSP/1.0") != 0) {
        return false;
    }

    char ip[20];
    uint16_t port = 0;
    char suffix[64] = {0};
    // for example 
    // 'rtsp://192.168.0.1:10000/test'              OPTIONS
    // 'rtsp://192.168.0.1:10000/test/track0'       SETUP
    if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {
        ;
    } else if (sscanf(url + 7, "%[^:]/%s", ip, suffix) == 2) {
        ;
    } else {
        return false;
    }
    
    // 对于发送sdp以前，mSuffix是session的名称，如，"test"，
    // 发送sdp以后的Setup请求，mSuffix包括track，如，"test/track0"
    // 因此对于有音视频双路的会话session，Setup请求会发送两次
    // 这取决于Sdp展示了多少路，以及客户端需要哪些
    mUrl = url;
    mSuffix = suffix; 
    return true;
}

bool RtspConnection::parseCSeqLine(const char* first, const char* last)
{
    const char* cseq = strstr(first, "CSeq:");
    if (!cseq || sscanf(cseq, "CSeq: %u", &mCSeq) != 1) {
        LOGI("parseCSeqLine failed, recived line: %s", cseq);
        mCSeq = -1;
        return false;
    }
    return true;
}

bool RtspConnection::parseLastContent(const char* first, const char* last)
{
    assert(mMethodType != NONE);
    if (!parseCSeqLine(first, last)) {
        LOGI("no CSeq found");
        return false;
    }
    if (mMethodType == OPTIONS) {
        return true;
    } else if (mMethodType == DESCRIBE) {
        return parseDescribe(first, last);
    } else if (mMethodType == SETUP) {
        return parseSetup(first, last);
    } else if (mMethodType == PLAY) {
        return parsePlay(first, last);
    } else if (mMethodType == PAUSE) {
        return parsePause(first, last);
    } else if (mMethodType == TEARDOWN) {
        return parseTeardown(first, last);
    } else {
        // ???????????????????????
        // exit(-1);
        LOGI("unsupoted method");
        return false;
    }
    // const char* crlf = NULL;
    // while (crlf = mInputBuffer.findNextCRLF())
    // {
    //     ;
    // }
    return false;
}

bool RtspConnection::parseDescribe(const char* first, const char* last)
{
    if (!strstr(first, "Accept:")) { // 
        return false;
    }
    if (!strstr(first, "application/sdp")) { // 
        return false;
    }
    return true;
}

bool RtspConnection::parseSetup(const char* first, const char* last)
{
    mTrackId = MediaSession::TrackNone;
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (mSuffix.find(mStreamPrefix + std::to_string(i)) != std::string::npos) {
            mTrackId = static_cast<MediaSession::TrackId>(i);
            break;
        }
    }
    if (mTrackId == MediaSession::TrackNone) {
        return false;
    }

    if (!strstr(first, "Transport:")) { // 不以"Accept:"开头
        return false;
    }
    if (strstr(first, "RTP/AVP/TCP")) { //
        ;
    } else if (strstr(first, "RTP/AVP")) {// 包含"RTP/AVP/UDP"
        if (strstr(first, "unicast")) {
            const char* client_port_str = strstr(first, "client_port=");
            // sscanf会忽视后面的不匹配的字符，例如用%d，则会忽视非数字和负号字符
            if (!client_port_str || sscanf(client_port_str, "client_port=%d-%d", &mDestRtpPort, &mDestRtcpPort) != 2) {
                    return false;
            }
        } else if (strstr(first, "multicast")) {
            // return true; // TODO 多播不用客户端端口号？！
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool RtspConnection::parsePlay(const char* first, const char* last)
{
    const char* str = strstr(first, "Session:");
    long sessionId;
    if (!str) {
        return false;
    }
    if (sscanf(str, "Session: %ld", &sessionId) != 1) {
        return false;
    }
    if (sessionId != mSessionId) {
        ;
    }
    return true;
}

bool RtspConnection::parsePause(const char* first, const char* last)
{
    return false;
}

bool RtspConnection::parseTeardown(const char* first, const char* last){
    // TODO 补充完整
    return true;
}

int RtspConnection::sendMessage()
{
    std::string sendMsg = std::string(mOutputBuffer.beginRead(), mOutputBuffer.beginWrite());
    LOGI("send msg: \n%s", sendMsg.c_str());
    int ret = mOutputBuffer.write(mClientFd);
    mOutputBuffer.retrievalAll();
    return ret;
}

int RtspConnection::sendMessage(void* buf, int len)
{
    mOutputBuffer.append(buf, len);
    return sendMessage();
}

bool RtspConnection::sendResponse()
{
    if (mMethodType == OPTIONS) {
        return sendOptions();
    } else if (mMethodType == DESCRIBE) {
        return sendDescribe();
    } else if (mMethodType == SETUP) {
        return sendSetup();
    } else if (mMethodType == PLAY) {
        return sendPlay(); 
    } else if (mMethodType == PAUSE) {
        return sendPause();
    } else if (mMethodType == TEARDOWN) {
        return sendTeardown();
    } 
    return false;
}

bool RtspConnection::sendOptions()
{
    // 准备内容
    mOutputBuffer.appendFormatted("RTSP/1.0 200 OK\r\n"
                        "CSeq: %u\r\n"
                        "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
                        "Server: %s\r\n"
                        "\r\n", mCSeq, SERVER_NAME);
    // 向客户端发送响应
    return sendMessage() < 0 ? false : true;
}

bool RtspConnection::sendDescribe()
{
    MediaSession* session = mServer->mediaSessionManager()->getSession(mSuffix);
    if (!session) {
        LOGE("session of %s don't found", mSuffix.c_str());
        return false;
    }
    mOutputBuffer.appendFormatted(
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Content-Base: %s\r\n" // 好像可以不发这个
        "Content-type: application/sdp\r\n"
        "Content-length: %u\r\n"
        "\r\n"
        "%s", 
        mCSeq, mUrl.c_str(), (int)session->generateSdpDescription().size(), session->generateSdpDescription().c_str()
    );
    // 向客户端发送响应
    return sendMessage() < 0 ? false : true;
}

bool RtspConnection::sendSetup()
{
    char sessionName[100];
    if (sscanf(mSuffix.c_str(), "%[^/]/", sessionName) != 1) { // "test/track0"zhong中的test
        // 没有解析到会话名称
        return false;
    }
    MediaSession* session = mServer->mediaSessionManager()->getSession(std::string(sessionName));
    if (!session) {
        return false;
    }
    if (mTrackId == MediaSession::TrackNone || mTrackId > MEDIA_MAX_TRACK_NUM || 
        mRtpInstances[mTrackId] || mRtcpInstances[mTrackId]) {
            return false;
    }

    if (session->isMulticast()) {
        mOutputBuffer.appendFormatted(
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Server: %s\r\n"
            "Transport: RTP/AVP;multicast;"
            "desitination:%s;source=%s;port=%d-%d,ttl=255\r\n"
            "Session: %08x\r\n"
            "\r\n", mCSeq, SERVER_NAME, session->getMulticastDestAddr().c_str(), sockets::getLocalIp().c_str(), 
            session->getMulticastDestRtpPort(mTrackId), session->getMulticastDestRtpPort(mTrackId) + 1, mSessionId
        );
    } else {
        if (mIsRtpOverTcp) {
            ;
        } else {
            // 创建服务端的rtp和rtcp端口
            if (!createRtpRtcpPortOverUdp(mTrackId, mDestIp, mDestRtpPort, mDestRtcpPort)) {
                LOGE("createRtpRtcpPortOverUdp failed");
                return false;
            }
            mRtpInstances[mTrackId]->setSessionId(mSessionId);
            mRtcpInstances[mTrackId]->setSessionId(mSessionId);

            session->addRtpInstance(mTrackId, mRtpInstances[mTrackId]);
            mOutputBuffer.appendFormatted(
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %u\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                "Session: %08x\r\n"
                "\r\n", mCSeq, SERVER_NAME, mDestRtpPort, mDestRtcpPort, 
                mRtpInstances[mTrackId]->getLocalPort(), mRtcpInstances[mTrackId]->getLocalPort(), 
                mSessionId); 
        }
    }

    return sendMessage() < 0 ? false : true;
}

bool RtspConnection::sendPlay()
{
    mOutputBuffer.appendFormatted(
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Server: %s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %08x; timeout=60\r\n"
        "\r\n", mCSeq, SERVER_NAME, mSessionId
    );
    if (sendMessage() < 0) {
        return false;
    }

    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) 
    {
        if (mRtpInstances[i]) {
            mRtpInstances[i]->setAlive(true);
        }
        if (mRtcpInstances[i]) {
            mRtcpInstances[i]->setAlive(true);
        }
    }
    return true;
}

bool RtspConnection::sendPause(){
    return false;
}

bool RtspConnection::sendTeardown()
{
    mOutputBuffer.appendFormatted(
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Server: %s\r\n"
        "\r\n", mCSeq, SERVER_NAME
    );
    if (sendMessage() < 0) {
        return false;
    }
    
    return true;
}

bool RtspConnection::createRtpRtcpPortOverUdp(MediaSession::TrackId trackId, std::string destIp, 
                                    uint16_t destRtpPort, uint16_t destRtcpPort)
{
    if (mRtpInstances[trackId]) {
        return false;
    }
    int rtpfd, rtcpfd;

    uint16_t port;
    uint16_t localRtpPort, localRtcpPort;
    int i = 10;
    while (i--) {
        rtpfd = sockets::createUdpSocket();
        if (rtpfd < 0) {
            return false;
        }
        rtcpfd = sockets::createUdpSocket();
        if (rtcpfd < 0) {
            sockets::close(rtpfd);
            return false;
        }
        port = rand() & 0xfffe; // 65534，求余是结果偶数
        if (port < 10000) {
            port += 10000;
        }
        localRtpPort = port;
        localRtcpPort = port + 1;
        if (!sockets::bind(rtpfd, "0.0.0.0", localRtpPort)) {
            sockets::close(rtpfd);
            sockets::close(rtcpfd);
            continue;
        }
        if (!sockets::bind(rtcpfd, "0.0.0.0", localRtcpPort)) {
            sockets::close(rtpfd);
            sockets::close(rtcpfd);
            continue;
        }
        break;
    }
    if (i == -1) {
        return false;
    }
    mRtpInstances[trackId] = RtpInstance::createNewOverUdp(rtpfd, localRtpPort, destIp, destRtpPort);
    mRtcpInstances[trackId] = RtcpInstance::createNew(rtcpfd, localRtcpPort, destIp, destRtcpPort);
    return true;
}
