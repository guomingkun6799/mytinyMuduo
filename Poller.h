//
// Created by MingkunGuo on 2026/10/14.
//
#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "EventLoop.h"
#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // Polls the IO events
    //给所有的IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    virtual void updateChannel(Channel* channel)=0;

    virtual void removeChannel(Channel* channel)=0;

    // 基类中已经实现（查 channels_ 表），不需要子类再 override，
    // 原来写成纯虚函数会导致 EPollPoller 变成抽象类、无法实例化。
    virtual bool hasChannel(Channel* channel);

    // EventLopp可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

    EventLoop* ownerLoop() const { return ownerloop_; }

protected:
    //  map的key: sockfd value: sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop* ownerloop_;
};

