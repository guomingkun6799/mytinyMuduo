//
// Created by MingkunGup on 2026/10/11.
//
#pragma once

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
