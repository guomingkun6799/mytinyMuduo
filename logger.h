//
// Created by MingkunGuo on 2026/10/11.
//

#ifndef MUDO_GM_LOGGER_H
#define MUDO_GM_LOGGER_H

#include <cstdio>
#include <cstdlib>
#include <string>

#include "noncopyable.h"

//定义日志的级别 INFO ERROR FATAL DEBUG
enum LogLevel {
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};

//输出一个日志类
class Logger : noncopyable {
public:
    //获取日志唯一的实例
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    //写日志
    void log(std::string msg);
private:
    int logLevel_;
    Logger() : logLevel_(INFO) {}
};

#define LOG_INFO(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char mudo_log_buf[1024]; \
        snprintf(mudo_log_buf, sizeof(mudo_log_buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(mudo_log_buf); \
    } while (0)

#define LOG_ERROR(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char mudo_log_buf[1024]; \
        snprintf(mudo_log_buf, sizeof(mudo_log_buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(mudo_log_buf); \
    } while (0)

#define LOG_FATAL(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char mudo_log_buf[1024]; \
        snprintf(mudo_log_buf, sizeof(mudo_log_buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(mudo_log_buf); \
        exit(-1); \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char mudo_log_buf[1024]; \
        snprintf(mudo_log_buf, sizeof(mudo_log_buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(mudo_log_buf); \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat,...) \
    do { (void)sizeof(logmsgFormat); } while (0)
#endif

#endif //MUDO_GM_LOGGER_H
