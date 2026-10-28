//
// Created by MingkunGuo on 2026/10/12.
//

#include "InetAddress.h"

#include <arpa/inet.h>     // inet_pton / inet_ntop 声明在这里
#include <strings.h>
#include <cstdio>
#include <cstring>
#include <string>

InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) != 1) {
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }
}

std::string InetAddress::toIp() const {
    char buffer[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));
    return buffer;
}

std::string InetAddress::toIpPort() const {
    char buffer[80] = {0}; 
    ::inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));
    size_t end = strlen(buffer);
    uint16_t port = ntohs(addr_.sin_port);
    ::snprintf(buffer + end, sizeof(buffer) - end, ":%u", port);
    return buffer;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}
