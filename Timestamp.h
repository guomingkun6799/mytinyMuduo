//
// Created by MingkunGuo on 2026/10/12.
//

#ifndef MUDO_GM_TIMESTAMP_H
#define MUDO_GM_TIMESTAMP_H
#include <cstdint>
#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};


#endif //MUDO_GM_TIMESTAMP_H
