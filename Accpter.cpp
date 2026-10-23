//
// Created by MingkunGuo on 2026/10/22.
//

#include "Accpter.h"
#include "logger.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h.h>

static int createNonblockingorIde() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_FATAL("%s : %s: %d listen socket creation failed", __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Accpter::Accpter(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop)
    , acceptSocket_(createNonblockingorIde())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer：：start() -> accpter::listen
    acceptChannel_.setReadCallback(
        std::bind(&Accpter::handleRead,this));
}

Accpter::~Accpter() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

// listenfd有新用户连接时发生
void Accpter::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr); //轮询找到subloop唤醒分发
        } else {
            ::close(connfd);
        }
    } else {
        LOG_ERROR("Accpter::handleRead: accept() failed\n");
        if (errno == EMFILE) {
            LOG_ERROR("sockfd reached limit\n");
        }
    }
}

void Accpter::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();// acceptChannel_ -> poller
}
