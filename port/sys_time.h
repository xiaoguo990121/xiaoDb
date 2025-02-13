#pragma once

#include "xiaodb/xiaodb_namespace.h"

#if defined(OS_WIN) && (defined(_MSC_VER) || defined(__MINGW32__))

#include <time.h>

namespace XIAODB_NAMESPACE
{
    namespace port
    {
        struct TimeVal
        {
            long tv_sec;
            long tv_usec;
        };

        void GetTimeofDay(TimeVal *tv, struct timezone *tz);

        inline struct tm *LocalTimeR(const time_t *timep, struct tm *result)
        {
            errno_t ret = localtime_s(result, timep);
            return (ret == 0) ? result : NULL;
        }
    }
}
#else
#include <sys/time.h>
#include <time.h>

namespace XIAODB_NAMESPACE
{
    namespace port
    {
        using TimeVal = struct timeval;

        inline void GetTimeOfDay(TimeVal *tv, struct timezone *tz)
        {
            gettimeofday(tv, tz);
        }

        inline struct tm *LocalTimeR(const time_t *timep, struct tm *result)
        {
            return localtime_r(timep, result);
        }
    }
}

#endif