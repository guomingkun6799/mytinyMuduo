//
// Created by MingkunGuo on 2026/10/15.
//

#include "EPollPoller.h"

#include <cstring>

#include "logger.h"
#include "channel.h"

#include <errno.h>

namespace {
    // channel未添加到poller中
    const int kNew = -1; // channel的成员 index_ = -1
    // channel已添加到poller中
    const int kAdded = 1;
    // channel从poller中删除
    const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create1 eroor:%d\n", errno);
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}
/**
 *  EventLoop包含： channelList poller， 所有的channel
 *  poller 包含 channelMap <fd channel*>, 向该poller注册过的fd。
 */
// channel update -> EventLoop updateChannel -> poller updatechannel
void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel -> index();
    LOG_INFO("fd = %d events=%d index=%d \n", channel -> fd(), channel ->events(), index);
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel -> fd();
            channels_[fd] = channel;
        }
        channel -> set_Index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // channel已经在poller上注册过了
        int fd = channel -> fd();
        if (channel -> isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel -> set_Index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel -> fd();
    int index = channel -> index();
    channels_.erase(fd);
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel -> set_Index(kNew);
}

// 更新channel通道，add/del/mod
void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    memset(&event, 0, sizeof(event));

    int fd = channel -> fd();

    event.events = channel -> events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del eroor:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
