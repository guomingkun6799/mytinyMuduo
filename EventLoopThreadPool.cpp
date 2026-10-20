//
// Created by MingkunGuo on 2026/10/20.
//

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <string>
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseloop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool(){}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;
    for (int i = 0; i < numThreads_; i++) {
        char buf[name_.size()+32];
        snprintf(buf, sizeof(buf), "%s%d",name.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t ->startLoop()); //底层创建线程，底层绑定一个新的EventLoop，并返回该地址
    }

    if (numThreads_ == 0 && cb) {
        cb(baseloop_);
    }
}

//轮询
EventLoop *EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = baseloop_;
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop *>(1, baseloop_);
    } else {
        loops_;
    }
}
