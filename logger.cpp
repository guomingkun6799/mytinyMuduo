//
// Created by ma-te on 2026/10/11.
//
#include <iostream>
#include <mutex>

#include "logger.h"
#include "Timestamp.h"

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level) {
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    switch (logLevel_) {
        case INFO:
            std::cout << "[INFO]";
            break;
        case ERROR:
            std::cout << "[ERROR]";
            break;
        case FATAL:
            std::cout << "[FATAL]";
            break;
        case DEBUG:
            std::cout << "[DEBUG]";
            break;
        default:
            break;
    }
    std::cout << Timestamp::now().toString() << ":" << msg << std::endl;
}
