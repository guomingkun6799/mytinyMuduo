//
// Created by MingkunGuo on 2026/10/12.
//

#ifndef MUDO_GM_INETADDRESS_H
#define MUDO_GM_INETADDRESS_H

#include <cstdint>
#include <netinet/in.h>
#include <string>

//封装socket地址类型
class InetAddress {
public:
    explicit InetAddress(uint16_t port=0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr):addr_(addr) {};

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in* getSockAddr() const {return addr_;};
private:
    sockaddr_in addr_;
};


#endif //MUDO_GM_INETADDRESS_H
