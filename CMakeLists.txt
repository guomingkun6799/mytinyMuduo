# ------------------------------------------------------------
# mudo_gm - 一个 muduo 风格的 C++ 网络库（构建中）
# 现代、可扩展的 CMake 配置
# ------------------------------------------------------------

# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.14)

# 项目定义
project(mudo_gm
    VERSION 0.1.0
    DESCRIPTION "muduo 风格的 C++ 基础/网络库"
    LANGUAGES CXX)

# ------------------------------------------------------------------
# 全局编译选项
# ------------------------------------------------------------------
# C++ 标准：muduo 风格代码通常需要 C++11 及以上
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建类型（单配置生成器如 Make/Ninja）
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "未指定构建类型，默认使用 Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# 输出目录统一管理
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 开启编译警告（MSVC / GCC / Clang）
if(MSVC)
    add_compile_options(/W4 /bigobj)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor
                        -Wold-style-cast -Woverloaded-virtual)
endif()

# 可选：开启 AddressSanitizer（Debug 下使用 -DSANITIZE=ON）
option(SANITIZE "Enable AddressSanitizer" OFF)
if(SANITIZE)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# ------------------------------------------------------------------
# 构建选项
# ------------------------------------------------------------------
option(BUILD_TESTS    "构建单元测试" OFF)
option(BUILD_EXAMPLES "构建示例代码" OFF)

# ------------------------------------------------------------------
# 库目标
# 当前项目仅有 header-only 的 noncopyable.h，
# 因此使用 INTERFACE 库；后续加入 .cc 源文件时可改为普通库。
# ------------------------------------------------------------------
file(GLOB_RECURSE MUDO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# 若存在源文件则构建为静态库，否则构建为 header-only 接口库
file(GLOB_RECURSE MUDO_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

if(MUDO_SOURCES)
    add_library(${PROJECT_NAME} STATIC ${MUDO_SOURCES} ${MUDO_HEADERS}
            InetAddress.cpp
            InetAddress.h
            TcpServer.cpp
            TcpServer.h)
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "以静态库方式构建 ${PROJECT_NAME}")
else()
    add_library(${PROJECT_NAME} INTERFACE
            logger.cpp
            logger.h
            Timestamp.cpp
            Timestamp.h
            InetAddress.cpp
            InetAddress.h
            TcpServer.cpp
            TcpServer.h
            EventLoop.cpp
            EventLoop.h
            Channel.cpp
            Channel.h
            Poller.cpp
            Poller.h
            DefaultPoller.cpp
            EPollPoller.cpp
            EPollPoller.h
            CurrentThread.cpp
            CurrentThread.h
            Thread.cpp
            Thread.h
            EventLoopThread.cpp
            EventLoopThread.h
            EventLoopThreadPool.cpp
            EventLoopThreadPool.h
            Socket.cpp
            Socket.h
            Accpter.cpp
            Accpter.h
            TcpConnection.cpp
            TcpConnection.h
            Callbacks.h
            buffer.cpp
            buffer.h
    )
    target_include_directories(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "以 header-only 接口库方式构建 ${PROJECT_NAME}")
endif()

# 导出编译命令（便于 clangd / 静态分析）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------
# 测试与示例（按需开启）
# ------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
