//
// Created by MingkunGuo on 2026/10/23.
//

#include "TcpConnection.h"
#include "logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>

static  EventLoop* CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d TcpConnection is null\n",__FILE__,__func__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int socketfd, const InetAddress &localaddr, const InetAddress &peeraddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(name)
    , state_(kConnecting)
    , socket_(new Socket(socketfd))
    , channel_(new Channel(loop, socketfd))
    , localaddr_(localaddr)
    , peeraddr_(peeraddr)
    , highWaterMark_(64*1024*1024) {
    //给channel设置相应的回调，poller给channel通知感兴趣的事件
    channel_ -> setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_ -> setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_ -> setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_ -> setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::TcpConnection [%s] in loop %p at fd:%d\n", name_.c_str(), loop_, sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::~TcpConnection [%s] at fd:%d state:%s\n", name_.c_str(), socket_->fd(), stateToString());
}