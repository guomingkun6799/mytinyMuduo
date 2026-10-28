//
// Created by MingkunGuo on 2026/10/12.
//

#include "TcpServer.h"
#include "logger.h"
#include "TcpConnection.h"

#include <cstdio>
#include <errno.h>
#include <functional>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static  EventLoop* CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null\n",__FILE__,__func__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    // option == kReusePort 时才复用端口，原来写反了
    , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , started_(0)
    , nextConnId_(1) {
    //当有新用户连接时，会执行newconnection回调
    acceptor_ -> setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this,std::placeholders::_1,std::placeholders::_2));
}

// 有一个新的客户端的连接,acceptor会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    EventLoop* ioLoop = threadPool_->getNextLoop();//轮询选择一个subloop
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection[%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(local));
    if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &addrlen) < 0) {
        LOG_ERROR("sockets::getLocalAddr failed, errno=%d\n", errno);
    }

    InetAddress localAddr(local);

    //创建连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            connName,
                            sockfd,
                            localAddr,
                            peerAddr
                        ));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::setThreadNum(int threadNum) {
    threadPool_ ->setThreadNum(threadNum);
}

void TcpServer::start() {
    if (started_++ == 0) { //防止一个TcpServer启动多次
        threadPool_ -> start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop[%s] - connenction %s\n", name_.c_str(),conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}

TcpServer::~TcpServer() {
    for (auto& item: connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}
