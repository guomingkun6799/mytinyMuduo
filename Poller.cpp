//
// Created by MingkunGuo on 2026/10/14.
//

#include "Poller.h"
#include "channel.h"

Poller::Poller(EventLoop* loop) :
     ownerloop_(loop)
{}

bool Poller::hasChannel(Channel *channel) {
     auto it = channels_.find(channel->fd());
     return it != channels_.end() && (it->second == channel);
}
