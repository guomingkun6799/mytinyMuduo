//
// Created by MingkunGuo on 2026/10/12.
//

#include "TcpServer.h"
#include "logger.h"

#include <functional>

EventLoop* CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null\n",__FILE__,__func__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , accpter_(new Accpter(loop, listenAddr, option == kNoReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1) {
    //当有新用户连接时，会执行newconnection回调
    accpter_ -> setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this,std::placeholders::_1,std::placeholders::_2));
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {

}

void TcpServer::setThreadNum(int threadNum) {
    threadPool_ ->setThreadNum(threadNum);
}

void TcpServer::start() {
    if (started_++ == 0) { //防止一个TcpServer启动多次
        threadPool_ -> start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Accpter::listen, accpter_.get()));
    }
}
