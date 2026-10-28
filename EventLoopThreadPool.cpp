//
// Created by MingkunGuo on 2026/10/20.
//

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <cstdio>
#include <memory>
#include <string>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseloop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;
    for (int i = 0; i < numThreads_; i++) {
        // 原来是变长数组 char buf[name_.size()+32]，C++ 标准不支持（GCC 扩展），
        // 而且 snprintf 里写的是未定义的 name，这里改成 std::string
        std::string threadName = name_ + std::to_string(i);
        EventLoopThread *t = new EventLoopThread(cb, threadName);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); //底层创建线程，底层绑定一个新的EventLoop，并返回该地址
    }

    if (numThreads_ == 0 && cb) {
        cb(baseloop_);
    }
}

//轮询
EventLoop *EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = baseloop_;
    if (!loops_.empty()) {
        loop = loops_[static_cast<size_t>(next_)];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop *>(1, baseloop_);
    }
    return loops_;   // 原来这里漏了 return
}
