#include <cassert>

#include "monitoring/perf_level_imp.h"

namespace XIAODB_NAMESPACE
{
    thread_local PerfLevel perf_level = kEnableCount;

    void SetPerfLevel(PerfLevel level)
    {
        assert(level > kUninitialized);
        assert(level < kOutOfBounds);
        perf_level = level;
    }

    PerfLevel GetPerfLevel() { return perf_level; }
}