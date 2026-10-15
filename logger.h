//
// Created by MingkunGuo on 2026/10/11.
//

#ifndef MUDO_GM_LOGGER_H
#define MUDO_GM_LOGGER_H

#include <string>

#include "noncopyable.h"

#define LOG_INFO(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024]; \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0)

#define LOG_ERROR(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024]; \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0)


#define LOG_FATAL(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024]; \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat,...) \
    do {\
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024]; \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat,...)
#endif

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
    Logger(){};
};


#endif //MUDO_GM_LOGGER_H
