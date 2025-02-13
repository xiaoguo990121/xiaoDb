#pragma once

#include "port/port.h"
#include "xiaodb/perf_level.h"

namespace XIAODB_NAMESPACE
{
    extern thread_local PerfLevel perf_level;
}