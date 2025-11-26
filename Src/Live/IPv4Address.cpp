#include "IPv4Address.h"

IPv4Address::IPv4Address(): mIp(""), mPort(-1)
{}

IPv4Address::IPv4Address(std::string ip, int port): mIp(ip), mPort(port)
{
    mAddr.sin_family = AF_INET; // 需要设置地址族，否则无法使用它发送信息
    mAddr.sin_port = htons(port);
    inet_aton(ip.c_str(), &(mAddr.sin_addr));
}

void IPv4Address::setAddr(std::string ip, int port)
{
    mIp = ip;
    mPort = port;
    mAddr.sin_family = AF_INET;
    mAddr.sin_port = htons(port);
    inet_aton(ip.c_str(), &(mAddr.sin_addr));
}

std::string IPv4Address::getIp()
{
    return mIp;
}

int IPv4Address::getPort()
{
    return mPort;
}

struct sockaddr* IPv4Address::getAddr()
{
    return (struct sockaddr*)&mAddr;
}
