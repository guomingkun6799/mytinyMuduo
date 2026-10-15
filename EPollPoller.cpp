//
// Created by MingkunGuo on 2026/10/15.
//

#include "EPollPoller.h"
#include "logger.h"

#include <errno.h>

namespace {
    const int kNew = -1;
    const int kAdded = 1;
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