//
// Created by MingkunGuo on 2026/10/22.
//

#include "Acceptor.h"
#include "logger.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

static int createNonblockingOrDie() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_FATAL("%s : %d: listen socket creation failed, errno=%d\n", __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop)
    , acceptSocket_(createNonblockingOrDie())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);   // 由 TcpServer 的 Option 决定，不能写死 true
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer：：start() -> acceptor::listen
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

// listenfd有新用户连接时发生
void Acceptor::handleRead() {
    // LT 模式下可以一次把已经就绪的连接全部取完，避免频繁触发事件
    for (;;) {
        InetAddress peerAddr;
        int connfd = acceptSocket_.accept(&peerAddr);
        if (connfd >= 0) {
            if (newConnectionCallback_) {
                newConnectionCallback_(connfd, peerAddr); //轮询找到subloop唤醒分发
            } else {
                ::close(connfd);
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;      // 没有更多的连接了，正常退出
            }
            if (errno == EINTR) {
                continue;   // 被信号打断，重试
            }
            if (errno == EMFILE) {
                LOG_ERROR("Acceptor::handleRead: sockfd reached limit\n");
            } else {
                LOG_ERROR("Acceptor::handleRead: accept() failed, errno=%d\n", errno);
            }
            break;
        }
    }
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();// acceptChannel_ -> poller
}
