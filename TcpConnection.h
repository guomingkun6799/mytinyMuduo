//
// Created by MingkunGuo on 2026/10/23.
//

#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer -> Accpter -> 有一个新用户连接，通过accept函数拿到connfd
 * -> TcpConnection设置回调 -> Channel -> Poller -> Channel回调
 */
class TcpConnection : noncopyable , public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop,
        const std::string& name,
        int socketfd,
        const InetAddress& localaddr,
        const InetAddress& peeraddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; };
    const std::string& name() const { return name_; };
    const InetAddress& localAddr() const { return localaddr_; };
    const InetAddress& peerAddr() const { return peeraddr_; };

    bool connected() const { return state_ == kConnected; };
    bool disconnected() const { return state_ == kDisconnected; };

    //发送数据
    void send(buffer* message);
    //关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& connectionCallback) {connectionCallback_ = connectionCallback;};
    void setMessageCallback(const MessageCallback& messageCallback) {messageCallback_ = messageCallback;};
    void setCloseCallback(const CloseCallback& closeCallback) {closeCallback_ = closeCallback;};
    void setWriteCompleteCallback(const WriteCompleteCallback& writeCompleteCallback) {writeCompleteCallback_ = writeCompleteCallback;};
    void setHighWaterMarkCallback(const HighWaterMarkCallback& highWaterMarkCallback) {highWaterMarkCallback_ = highWaterMarkCallback;};

    //连接建立
    void connectEstablished();
    void connectDestroyed();

private:
    enum State {
        kDisconnected, kConnecting, kConnected, kDisconnecting
    };

    void handleRead(Timestamp receiveTimestamp);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // 这里绝对不是baseloop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localaddr_;
    const InetAddress peeraddr_;

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完之后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    buffer inputBuffer_;
    buffer outputBuffer_;
};
