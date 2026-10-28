//
// Created by MingkunGuo on 2026/10/12.
//

#ifndef MUDO_GM_TIMESTAMP_H
#define MUDO_GM_TIMESTAMP_H

#include <cstdint>
#include <ctime>
#include <string>

// 时间戳，内部统一使用「微秒」存储
class Timestamp {
public:
    static constexpr int64_t kMicroSecondsPerSecond = 1000 * 1000;

    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }

    std::string toString() const;        // 可读格式：2026/10/12 10:00:00.123456

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

private:
    int64_t microSecondsSinceEpoch_;
};

#endif //MUDO_GM_TIMESTAMP_H
