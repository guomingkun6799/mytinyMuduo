//
// Created by MingkunGuo on 2026/10/23.
//

#include "TcpConnection.h"
#include "logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <io.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <strings.h>


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
    LOG_INFO("TcpConnection::TcpConnection [%s] in loop %p at fd:%d\n", name_.c_str(), loop_, socketfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::~TcpConnection [%s] at fd:%d state:%s\n", name_.c_str(), socket_->fd(), stateToString());
}

void TcpConnection::handleRead(Timestamp receiveTimestamp) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调
        messageCallback_(std::shared_from_this(), &inputBuffer_, receiveTimestamp);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, std::shared_from_this())
                    );
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    } else {
        LOG_ERROR("TcpConnection fd = %d is down, no more writing",socket_->fd());
    }
}

void TcpConnection::handleClose() {
    LOG_INFO("fd = %d state= %d \n", channel_->fd(), state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(std::shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
        LOG_ERROR("TcpConnection::handleError");
    } else {
        err = optval;
        LOG_ERROR("TcpConnection::handleError SO_ERROR=%d %s", err, strerror_tl(err));
    }
}

void TcpConnection::send(const std::string &buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(),buf.size());
        } else {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

//发送数据
void TcpConnection::sendInLoop(const void *data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    //表示channel_第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_ -> isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote > 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                //在这里数据全部发送完成，就不用再给channel设置epollout事件
               loop_->queueInLoop(
                   std::bind(writeCompleteCallback_, std::shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 注册epollout事件， poller发现发送tcp缓冲区有空间，会通知相应的sock -> channel调用hanlewrite回调
    if (!faultError && remaining > 0) { //说明这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区中
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_) {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, std::shared_from_this(), oldLen + remaining )
            );
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!channel_ -> isWriting()) {
            channel_->enableWriting(); //这里一定要注册channel的写事件
        }
    }
}

void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(std::shared_from_this());
    channel_->enableReading();

    connectionCallback_(std::shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(std::shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    if (!channel_ -> isWriting()) { //说明当前outputbuffer中的数据已经全部发送完成
        socket_->shutdownWrite();
    }
}
