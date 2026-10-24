//
// Created by MingkunGuo on 2026/10/23.
//

#pragma once

#include <functional>
#include <memory>

#include "Timestamp.h"

class buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

using MessageCallback = std::function<void(
    const TcpConnectionPtr&, buffer*, Timestamp)>;