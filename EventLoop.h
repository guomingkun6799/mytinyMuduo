//
// Created by MingkunGuo on 2026/10/12.
//
#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <sys/types.h>
#include <memory>
#include <mutex>
#include <thread>

class Channel;
class Poller;

// 事件循环类，包含：channel,Poller(epoll的抽象)
class EventLoop : noncopyable{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程的
    void wakeup();

    // EventLoop的方法 -> Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断Eventloop对象是否在自己的小县城里面
    bool isInLoopThread() const {return threadId_== CurrentThread::tid();};
private:
    void handleRead(); // wakeup
    void doPendingFunctors(); // 执行后调

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_; //记录当前loop所在线程的id

    Timestamp pollReturnTime_; //poller返回发生时间的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; //当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员通知唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; //存储这个loop需要执行的所有回调操作
    std::mutex mutex_; //互斥锁，用来保护上面vector的安全操作

};

