//
// Created by MingkunGuo on 2026/10/12.
//
#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

//对外的服务器编程使用的类
class TcpServer : noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
        const InetAddress& listenAddr,
        const std::string& name,
        Option option = Option::kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &callback) {threadInitCallback_ = callback;}
    void setConnectionCallback(const ConnectionCallback &callback) {connectionCallback_ = callback;}
    void setMessageCallback(const MessageCallback &callback) {messageCallback_ = callback;}
    void setWriteCompleteCallback(const WriteCompleteCallback &callback) {writeCompleteCallback_ = callback;}

    //相当于设置subloop的个数
    void setThreadNum(int threadNum);

    //开启服务器监听
    void start();
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop *loop_; // baseloop用户定义的loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; //运行在mainLoop, 任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完之后的回调
    ThreadInitCallback threadInitCallback_;//loop线程初始化的回调
    std::atomic_int32_t started_;

    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接
};
