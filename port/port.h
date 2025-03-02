#pragma once

#include <string>
#define OS_WIN
#if defined(XIAODB_PLATFORM_POSIX)
#include "port/port_posix.h"
#elif defined(OS_WIN)
#include "port/win/port_win.h"
#endif

#ifdef OS_LINUX
extern "C" bool XiaoDbThreadYieldAndCheckAbort() __attribute__((__weak__));
#define XIAODB_THREAD_YIELD_CHECK_ABORT() \
    (XiaoDbThreadYieldAndCheckAbort ? XiaoDbThreadYieldAndCheckAbort() : false)
#else
#define XIAODB_THREAD_YIELD_CHECK_ABORT() (false)
#endif