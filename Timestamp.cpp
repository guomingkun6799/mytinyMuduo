//
// Created by MingkunGuo on 2026/10/12.
//

#include "Timestamp.h"

#include <time.h>
#include <sys/time.h>

#include <cstdio>

Timestamp::Timestamp()
    : microSecondsSinceEpoch_(0) {
}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {
}

Timestamp Timestamp::now() {
    // 原来的实现用的是 time(NULL)，只有「秒」精度，
    // 而成员变量叫 microSecondsSinceEpoch_，单位对不上，这里统一成微秒。
    struct timeval tv;
    ::gettimeofday(&tv, nullptr);
    return Timestamp(static_cast<int64_t>(tv.tv_sec) * kMicroSecondsPerSecond +
                     static_cast<int64_t>(tv.tv_usec));
}

std::string Timestamp::toString() const {
    char buffer[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);

    struct tm tm_time;
    // localtime 返回的是静态缓冲区，多线程下不安全，这里用 localtime_r
    ::localtime_r(&seconds, &tm_time);

    ::snprintf(buffer, sizeof(buffer), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
               microseconds);
    return buffer;
}
