//
// Created by MingkunGuo on 2026/10/14.
//

#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        // 预留：以后实现 PollPoller 时在这里返回 new PollPoller(loop)
        return new EPollPoller(loop);
    }
    return new EPollPoller(loop);
}
