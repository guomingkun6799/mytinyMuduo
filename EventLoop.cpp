//
// Created by MingkunGuo on 2026/10/12.
//

#include "EventLoop.h"
#include "logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

//防止一个线程创建多个EventLoop thread_local
__thread EventLoop *t_loopInthisThread = nullptr;

const int kPollTimeMs = 10000; //默认的poller io复用接口的超时时间

// 创建wakeupfd， 用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr) {
    LOG_DEBUG("Eventloop created %p in thread %d", this, threadId_);
    if (t_loopInthisThread) {
        LOG_FATAL("Another EventLoop %p created in thread %d",this, threadId_);
    } else {
        t_loopInthisThread = this;
    }
    //设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_ -> setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每个eventloop都将监听wakeupchannel的epollin事件
    wakeupChannel_ -> enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_ -> disableAll();
    wakeupChannel_ -> remove();
    ::close(wakeupFd_);
    t_loopInthisThread = nullptr;
}

//开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_INFO("Eventloop %p start looping\n", this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_ ->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel: activeChannels_) {
            //Poller监听哪些channel发生事件了，然后上报给evnetloop， 通知channel处理相应的事件
            channel -> handleEvent(pollReturnTime_);
        }

        //执行当前eventloop事件循环需要处理的回调操作
        /**
         *  IO线程 mainLoop 接收新用户的连接 fd(channel) subloop
         *  mainloop事先注册一个回调cb(需要subloop来执行) wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
         */
        doPendingFunctors();
    }

    LOG_INFO("Eventloop %p stop looping\n", this);
    looping_ = false;
}

//退出事件循环： 1、loop在自己的线程中调用quit
void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) { // 2、在其他线程中调用的quit，
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup(); //唤醒loop所在线程
    }
}

//执行回调
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const auto& functor : functors) {
        functor(); //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead returned %d", n);
    }
}

//向wakefd_写一个数据,用来唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup returned %d", n);
    }
}