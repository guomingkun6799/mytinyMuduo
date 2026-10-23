//
// Created by MingkunGuo on 2026/10/22.
//

#include "Socket.h"
#include "logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <strings.h>

Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr) {
    if (::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)) != 0) {
        LOG_FATAL("bind socketfd:%d failed\n", sockfd_);
    }
}

void Socket::listen() {
    if (::listen(sockfd_, 1024) != 0) {
        LOG_FATAL("listen socketfd:%d failed\n", sockfd_);
    }
}

/**
 * 接受一个客户端连接，并把对端的地址保存到 peeraddr 中
 * 成功返回已连接的 connfd，失败返回 -1（不做FATAL，交由上层处理）
 */
int Socket::accept(InetAddress *peeraddr) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);   // 必须初始化为缓冲区大小，否则accept可能写越界
    bzero(&addr, sizeof(addr));

    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() {
    // 只关闭写端，还能继续读对方发来的数据（TCP半关闭）
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR("shutdown write socketfd:%d failed\n", sockfd_);
    }
}

void Socket::setTcpNoDelay(bool on) {
    // 禁用Nagle算法：小数据包立即发送，不攒包，降低延迟
    int optval = on ? 1 : 0;
    if (::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) != 0) {
        LOG_ERROR("setsockopt TCP_NODELAY socketfd:%d failed\n", sockfd_);
    }
}

void Socket::setReuseAddr(bool on) {
    // 允许重用处于TIME_WAIT状态的地址，服务器重启时不必等待
    int optval = on ? 1 : 0;
    if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        LOG_ERROR("setsockopt SO_REUSEADDR socketfd:%d failed\n", sockfd_);
    }
}

void Socket::setReusePort(bool on) {
    // 允许多个socket绑定同一端口（配合多线程/多进程accept负载均衡）
    int optval = on ? 1 : 0;
    if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) != 0) {
        LOG_ERROR("setsockopt SO_REUSEPORT socketfd:%d failed\n", sockfd_);
    }
}

void Socket::setKeepAlive(bool on) {
    // 开启TCP层的心跳保活，长时间无数据交互也能检测对端是否存活
    int optval = on ? 1 : 0;
    if (::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) != 0) {
        LOG_ERROR("setsockopt SO_KEEPALIVE socketfd:%d failed\n", sockfd_);
    }
}