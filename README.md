# mudo

> 一个用于学习的迷你 muduo：从零手写的 C++ 网络库，完整复刻 muduo 的 **one loop per thread + 主从 Reactor** 架构。
>
## 已实现的功能

### 1. Reactor 核心

| 组件 | 说明 |
| --- | --- |
| `EventLoop` | 事件循环本体。`loop()` 驱动 poller 取事件并分发；提供 `runInLoop` / `queueInLoop` 支持跨线程投递任务；用 `eventfd` 实现 `wakeup()`，唤醒阻塞在 `epoll_wait` 上的线程；`quit()` 支持跨线程安全退出 |
| `Channel` | 一个 fd 的事件封装。登记关心的事件（`EPOLLIN` / `EPOLLOUT` / `EPOLLPRI` / `EPOLLRDHUP`）并绑定读、写、关闭、错误四个回调；`tie()` 用 `weak_ptr` 解决"Channel 已被移除但回调还在执行"的生命周期问题 |
| `Poller` | IO 多路复用的抽象基类，统一 `poll` / `updateChannel` / `removeChannel` / `hasChannel` 接口，内部维护 `fd -> Channel*` 映射 |
| `EPollPoller` | `epoll` 实现。`epoll_create1(EPOLL_CLOEXEC)` 建实例，`epoll_ctl` 做 add/mod/del，`epoll_wait` 取事件；事件数组按需翻倍扩容；采用 **LT（水平触发）** 模式 |

### 2. 线程模型

| 组件 | 说明 |
| --- | --- |
| `Thread` | `std::thread` 的轻量封装，`promise/future` 同步保证 `start()` 返回时子线程已完成 tid 初始化；支持 `join` / `detach`、自动命名 `Thread1`、`Thread2`… |
| `CurrentThread` | 用 `__thread` 缓存线程 tid，避免每次 `syscall(SYS_gettid)` |
| `EventLoopThread` | "启动一个线程并在其中运行一个 EventLoop"，用 `mutex + condition_variable` 等待 loop 创建完成再返回指针 |
| `EventLoopThreadPool` | subReactor 线程池，`setThreadNum(n)` 创建 n 个 loop 线程，`getNextLoop()` 以**轮询**方式把新连接分发给 subLoop；`n == 0` 时退化为单线程（全部在 baseLoop 上跑） |

### 3. TCP 服务端

| 组件 | 说明 |
| --- | --- |
| `Socket` | fd 的 RAII 封装。`accept` 使用 `accept4` 一次性设置 `SOCK_NONBLOCK \| SOCK_CLOEXEC`；提供 `setReuseAddr` / `setReusePort` / `setKeepAlive` / `setTcpNoDelay` / `shutdownWrite` |
| `Acceptor` | 监听套接字 + `Channel`。`listen()` 后注册读事件，可读时**循环 accept** 直到 `EAGAIN`，把新连接回调给 `TcpServer` |
| `InetAddress` | `sockaddr_in` 封装，支持 `toIp()` / `toIpPort()` / `toPort()` |
| `TcpConnection` | 连接对象，`shared_ptr` 管理。内含**状态机**（`kConnecting` / `kConnected` / `kDisconnecting` / `kDisconnected`）、输入输出缓冲区、发送**高水位回调**；`send()` 线程安全，非本 loop 线程时自动转投；输出缓冲区未发完时自动注册 `EPOLLOUT`，写完后回调 `writeCompleteCallback` |
| `TcpServer` | 对外门面。持有 `Acceptor`、线程池和 `name -> TcpConnectionPtr` 连接表；`start()` 幂等，新连接到来时选一个 subLoop 并在其上 `connectEstablished()` |
| `buffer` | 网络缓冲区。`readerIndex_` / `writerIndex_` 双下标 + 8 字节 `kCheapPrepend`；`readFd()` 使用 `readv` 配合栈上 64KB 临时缓冲区，一次读取即可应对大包，避免频繁扩容 |

### 4. 基础设施

- `Timestamp`：微秒精度时间戳（`gettimeofday`），`toString()` 用 `localtime_r` 输出可读时间。
- `logger`：`LOG_INFO` / `LOG_ERROR` / `LOG_FATAL` / `LOG_DEBUG` 宏，输出级别、时间与消息，多线程下加锁避免交错。
- `Callbacks.h`：集中定义 `ConnectionCallback` / `MessageCallback` / `WriteCompleteCallback` / `CloseCallback` / `HighWaterMarkCallback` 等回调类型。
- `noncopyable`：禁止拷贝的基类。

## 数据流

```
                  ┌──────────────── baseLoop (mainReactor) ────────────────┐
  新连接 ─────────▶│  Acceptor::handleRead ──▶ TcpServer::newConnection     │
                  │        │                        │                     │
                  │        └── accept4 ─────────────┘                     │
                  └───────────────────┬───────────────────────────────────┘
                                      │ getNextLoop() 轮询
                    ┌─────────────────▼─────────────────┐
                    │   subLoop (subReactor) × N        │
                    │   TcpConnection ──▶ Channel        │
                    │        ▲                │          │
                    │        │         EPollPoller        │
                    │        └──── handleEvent ◀─────────┘
                    └───────────────────────────────────┘
```

1. `Acceptor` 在 baseLoop 上收到新连接，`accept4` 得到非阻塞 `connfd`；
2. `TcpServer` 轮询选一个 subLoop，创建 `TcpConnection` 并 `runInLoop(connectEstablished)`；
3. 之后该连接的所有读写事件都在这个 subLoop 线程内处理，无需加锁；
4. 连接关闭时 `TcpConnection::handleClose` → `TcpServer::removeConnection` → 回到 baseLoop 删除连接表项，再让 IO 线程执行 `connectDestroyed` 摘掉 Channel。

## 目录结构

```
.
├── noncopyable.h          # 禁止拷贝基类
├── Timestamp.{h,cpp}      # 微秒时间戳
├── logger.{h,cpp}         # 简易日志宏
├── CurrentThread.{h,cpp}  # 线程 tid 缓存
├── Thread.{h,cpp}         # std::thread 封装
├── EventLoop.{h,cpp}      # 事件循环
├── Channel.{h,cpp}        # fd 事件封装
├── Poller.{h,cpp}         # IO 复用抽象
├── DefaultPoller.cpp      # 默认 Poller 工厂
├── EPollPoller.{h,cpp}    # epoll 实现
├── EventLoopThread.{h,cpp}      # loop + thread
├── EventLoopThreadPool.{h,cpp}  # subReactor 线程池
├── InetAddress.{h,cpp}    # 地址封装
├── Socket.{h,cpp}         # socket fd 封装
├── Acceptor.{h,cpp}       # 监听器
├── buffer.{h,cpp}         # 网络缓冲区
├── TcpConnection.{h,cpp}  # 连接对象
├── TcpServer.{h,cpp}      # 服务端门面
├── Callbacks.h            # 回调类型定义
└── CMakeLists.txt
```

## 构建

依赖：Linux（`epoll` / `eventfd`）、支持 C++17 的编译器、CMake ≥ 3.14。

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
```

产物为静态库 `build/lib/libmudo_gm.a`。

可选选项：

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `-DSANITIZE=ON` | OFF | 开启 AddressSanitizer，排查内存问题 |
| `-DBUILD_TESTS=ON` | OFF | 构建 `tests/` 下的测试 |
| `-DBUILD_EXAMPLES=ON` | OFF | 构建 `examples/` 下的示例 |

## 最小示例：Echo 服务器

```cpp
#include "EventLoop.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "buffer.h"
#include "logger.h"

#include <string>

int main() {
    EventLoop loop;                      // baseLoop
    InetAddress listenAddr(8000);        // 0.0.0.0:8000
    TcpServer server(&loop, listenAddr, "EchoServer");

    server.setThreadNum(4);              // 4 个 subReactor
    server.setConnectionCallback([](const TcpConnectionPtr& conn) {
        LOG_INFO("%s %s\n", conn->name().c_str(),
                 conn->connected() ? "up" : "down");
    });
    server.setMessageCallback(
        [](const TcpConnectionPtr& conn, buffer* buf, Timestamp) {
            std::string msg = buf->retrieveAllAsString();
            conn->send(msg);             // 原样回写；线程安全
        });

    server.start();
    loop.loop();                         // 阻塞直到 quit()
    return 0;
}
```

链接方式：`target_link_libraries(your_app PRIVATE mudo_gm)`。

## 建议的阅读顺序

1. `Timestamp` / `logger` / `noncopyable` —— 热身。
2. `Channel` + `Poller` + `EPollPoller` —— 理解"fd 如何变成事件"。
3. `EventLoop` —— 理解"事件如何被取出并分发"，重点看 `wakeup` 与 `doPendingFunctors`。
4. `Thread` / `CurrentThread` / `EventLoopThread` / `EventLoopThreadPool` —— 理解 one loop per thread。
5. `Socket` / `InetAddress` / `Acceptor` —— 理解监听与 accept。
6. `buffer` —— 理解读写的缓冲区设计。
7. `TcpConnection` —— 状态机、发送流程、生命周期（`tie` + `shared_from_this`）。
8. `TcpServer` —— 把上面所有东西串起来。

## 说明

本项目仅用于学习与交流，接口与实现大量参考 muduo（陈硕，MIT License）。生产环境请直接使用 muduo 或更新的事件驱动网络框架。
