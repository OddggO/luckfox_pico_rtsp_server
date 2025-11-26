#pragma once
#include <string>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <arpa/inet.h>

class IPv4Address
{
public:
    IPv4Address();  
    IPv4Address(std::string ip, int port);
    void setAddr(std::string ip, int port);
    std::string getIp();
    int getPort();
    struct sockaddr* getAddr();
private:
    std::string mIp;
    int mPort;
    struct sockaddr_in mAddr;
};