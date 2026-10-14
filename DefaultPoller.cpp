//
// Created by MingkunGuo on 2026/10/14.
//

#include "Poller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POOL")) {
        //生成poll实例
        return nullptr;
    } else {
        //生成epoll实例
        return nullptr;
    }
}
