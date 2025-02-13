#pragma once

#define XIAO_LOG_STRINGIFY(x) #x
#define XIAO_LOG_TOSTRING(x) XIAO_LOG_STRINGIFY(x)
#define XIAO_LOG_PREPEND_FILE_LINE(FMT) \
    ("[%s:" XIAO_LOG_TOSTRING(__LINE__) "] " FMT)

inline const char *XiaoLogShorterFileName(const char *file)
{
    // 18 is the length of "logging/logging.h".
    // If the name of this file changed, please change this number, too.
    return file + (sizeof(__FILE__) > 18 ? sizeof(__FILE__) - 18 : 0);
}

#define XIAO_LOG_HEADER(LGR, FMT, ...) \
    XIAODB_NAMESPACE::Log(InfoLogLevel::HEADER_LEVEL, LGR, FMT, ##__VA_ARGS__)

#define XIAO_LOG_AT_LEVEL(LGR, LVL, FMT, ...)                            \
    XIAODB_NAMESPACE::Log((LVL), (LGR), XIAO_LOG_PREPEND_FILE_LINE(FMT), \
                          XiaoLogShorterFileName(__FILE__), ##__VA_ARGS__)

#define XIAO_LOG_DEBUG(LGR, FMT, ...) \
    XIAO_LOG_AT_LEVEL((LGR), InfoLogLevel::DEBUG_LEVEL, FMT, ##__VA_ARGS__)

#define XIAO_LOG_INFO(LGR, FMT, ...) \
    XIAO_LOG_AT_LEVEL((LGR), InfoLogLevel::INFO_LEVEL, FMT, ##__VA_ARGS__)

#define XIAO_LOG_WARN(LGR, FMT, ...) \
    XIAO_LOG_AT_LEVEL((LGR), InfoLogLevel::WARN_LEVEL, FMT, ##__VA_ARGS__)

#define XIAO_LOG_ERROR(LGR, FMT, ...) \
    XIAO_LOG_AT_LEVEL((LGR), InfoLogLevel::ERROR_LEVEL, FMT, ##__VA_ARGS__)

#define XIAO_LOG_FATAL(LGR, FMT, ...) \
    XIAO_LOG_AT_LEVEL((LGR), InfoLogLevel::FATAL_LEVEL, FMT, ##__VA_ARGS__)

#define XIAO_LOG_BUFFER(LOG_BUF, FMT, ...)                                  \
    XIAODB_NAMESPACE::LogToBuffer(LOG_BUF, XIAO_LOG_PREPEND_FILE_LINE(FMT), \
                                  XiaoLogShorterFileName(__FILE__),         \
                                  ##__VA_ARGS__)

#define XIAO_LOG_BUFFER_MAX_SZ(LOG_BUF, MAX_LOG_SIZE, FMT, ...) \
    XIAODB_NAMESPACE::LogToBuffer(                              \
        LOG_BUF, MAX_LOG_SIZE, XIAO_LOG_PREPEND_FILE_LINE(FMT), \
        XiaoLogShorterFileName(__FILE__), ##__VA_ARGS__)

#define XIAO_LOG_DETAILS(LGR, FMT, ...) \
    ; // due to overhead by default skip such lines
// XIAO_LOG_DEBUG(LGR, FMT, ##__VA_ARGS__)
