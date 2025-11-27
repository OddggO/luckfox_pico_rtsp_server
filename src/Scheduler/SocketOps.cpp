#include "SocketOps.h"
#include "Log.h"
#include <unistd.h>
#include <fcntl.h>

int sockets::createTcpSocket()
{
    // TODO 为了搭配epoll/select等函数使用，是否应该使用SOCK_NONBLOCK使得套接字非阻塞？
    // GPT解释说是应该的，因为有非常低的可能，当数据不足时，read()发生阻塞
    // 根据AF_INET和SOCK_STREAM可以确定是TCP，因此第三个可写可不写（不写是直接填0）
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    // int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (fd < 0) {
        LOGE("tcp socket error\n");
        return fd;
    }
    return fd;
}

int sockets::createUdpSocket()
{
    int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        LOGE("udp socket error\n");
        return fd;
    }
    return fd;
}

bool sockets::bind(int sockfd, std::string ip, uint16_t port)
{
    if (sockfd < 0 || ip.empty() || port <= 0) {
        return false;
    }
    // 也可以用inet_addr给addr.sin_addr.s_addr(in_addr_t)赋值，但是这个函数已经被视为不全
    sockaddr_in addr;   
    addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(addr.sin_addr));
    addr.sin_port = htons(port);
    if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(sockaddr_in)) < 0) {
        LOGE("::bind(sockfd, (struct sockaddr*)&addr, sizeof(sockaddr_in)) < 0 error, %s:%d", ip.c_str(), port);
        return false;
    }
    return true;
}

bool sockets::listen(int sockfd, int backlog)
{
    if (::listen(sockfd, backlog) < 0) {
        LOGE("::listen(sockfd, backlog) < 0 error, %d, %d", sockfd, backlog);
        return false;
    }
    return true;
}

bool sockets::reUseAddr(int sockfd)
{
    int on = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    return true;
}

int sockets::accept(int lfd)
{
    struct sockaddr_in addr = {0};
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int connfd = ::accept(lfd, (struct sockaddr*)&addr, &addrLen);
    setNonBlockAndCloseOnExec(connfd);
    // ignoreSigPipeOnSocket(connfd); // 关闭套接字触发SIGPIPE，这里似乎是错误的，要在sendto里面用MSG_NOSIGNAL，而不是setsocket
    return connfd;
}

void sockets::setNonBlockAndCloseOnExec(int sockfd)
{
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
}

void sockets::ignoreSigPipeOnSocket(int sockfd)
{
    int option = 1;
    // TODO 这里似乎是错误的，要在sendto里面用MSG_NOSIGNAL，而不是setsocket
    setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
}

int sockets::read(int fd, char* buf, int len)
{
    return ::read(fd, buf, len);
}

int sockets::write(int fd, const void* buf, int len)
{
    return ::write(fd, buf, len); // 通过::区分系统函数和自己写的同名函数
}

int sockets::sendto(int fd, const void* buf, int len, const struct sockaddr* deskAddr)
{
    if (!deskAddr) {
        LOGE("deskAddr is nullptr");
        return -1;
    }
    socklen_t addrLen = sizeof(struct sockaddr);
    // LOGI("addr->sa_family = %d", deskAddr->sa_family);
    return ::sendto(fd, (char*)buf, len, 0, deskAddr, addrLen);
}

int sockets::close(int fd)
{
    return ::close(fd);
}

std::string sockets::getLocalIp()
{
    return "0.0.0.0";
}
