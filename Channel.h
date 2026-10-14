//
// Created MingkunGuo on 2026/10/12.
//
#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;
/**
 * Channel 理解为通道，封装了sockfd以及其感兴趣的event如epoll in、epoll out事件
 * 还绑定了poller返回的具体事件
 */
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件。调用相应的方法
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb);};
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb);};
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb);};
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb);};

    //防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; };
    int events() const { return events_; };
    void set_revents(int revents) { revents_ = revents; }; // poller监听
    bool isNoneEvent() const {return events_ == kNoneEvent;};

    //设置fd响应的事件状态
    void enableReading() { events_ |= kReadEvent; update();};
    void disableReading() { events_ &= ~kReadEvent; update();};
    void enableWriting() { events_ |= kWriteEvent; update();};
    void disableWriting() { events_ &= ~kWriteEvent; update();};
    void disableAll() { events_ = kNoneEvent; update();};
    bool isWriting() const { return events_ & kWriteEvent;};
    bool isReading() const { return events_ & kReadEvent;};

    int index() const { return index_; };
    void set_Index(int index) { index_ = index; };

    //one loop per thread
    EventLoop* ownerLoop() {return loop_};
    void remove();

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; //事件循环
    const int fd_; //fd，Poller监听的对象
    int events_; //注册fd感兴趣的事件
    int revents_; //poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体事件的revents，所以他负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};


