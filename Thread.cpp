//
// Created by MingkunGuo on 2026/10/20.
//

#include "Thread.h"
#include "CurrentThread.h"

#include <cstdio>
#include <future>
#include <utility>

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name) {
    setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_ && thread_ && thread_->joinable()) {
        thread_ -> detach(); //设置分离线程
    }
}

void Thread::start() {
    started_ = true;

    // 用 promise/future 代替原来的 sem_t：
    // 原实现里 sem 是 start() 栈上的局部变量，却被新线程的 lambda 按引用捕获，
    // start() 返回后 sem 已析构，属于悬垂引用/未定义行为。
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    thread_ = std::make_shared<std::thread>(
        [this, p = std::move(promise)]() mutable {
            tid_ = CurrentThread::tid();   // 在新线程中记录自己的 tid
            p.set_value();                 // 通知 start() 可以返回了
            func_();
        });
    future.wait();
}

void Thread::join() {
    joined_ = true;
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}
