//
// Created by MingkunGuo on 2026/10/12.
//

#include "Channel.h"
#include "EventLoop.h"
#include "logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{}
Channel::~Channel(){}

// Channel的tie方法什么时候调用
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
};

//当改变channel所表示的fd的events事件后， update负责在poller里面更改fd对应的事件epoll_ctl
void Channel::update() {
    //通过channel所属于的EventLoop调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中，把当前的channel删除掉
void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

//根据poller监听的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvents revents:%d\n", revents_);
    // EPOLLHUP：对端关闭；此时如果没有可读事件，直接走关闭回调
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    // EPOLLRDHUP：对端半关闭（收到 FIN），同样按可读处理，交给上层 read 返回 0
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}