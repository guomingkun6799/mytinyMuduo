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

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead returned %d", n);
    }
}

