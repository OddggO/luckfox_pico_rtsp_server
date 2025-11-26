#pragma once
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace sockets{
    int createTcpSocket(); // 默认创建非阻塞的tcp套接字
    int createUdpSocket(); // 默认创建非阻塞的udp套接字
    bool bind(int sockfd, std::string ip, uint16_t port);
    bool listen(int sockfd, int backlog);
    bool reUseAddr(int sockfd);
    int accept(int lfd); // 返回值为监听套接字
    void setNonBlockAndCloseOnExec(int sockfd); // 为套接字设置非阻塞和
    void ignoreSigPipeOnSocket(int sockfd); 
    int read(int fd, char* buf, int len);
    // 一般来说，write/send用于TCP，sendto用于UDP，因为TCP服务端在bind时就已经确定了ip和端口，
    // 服务端在accept后也能知道客户端的ip和端口
    int write(int fd, const void* buf, int len);
    int sendto(int fd, const void* buf, int len, const struct sockaddr* deskAddr);
    int close(int fd);
    std::string getLocalIp();
}