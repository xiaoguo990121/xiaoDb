#pragma once

#include <stdint.h>

#include <string>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    // How much perf stats to collect. Affects perf_context and iostats_context.
    // These levels are incremental, which means a new set of metrics will get
    // collected when PerfLevel is upgraded from level n to level n + 1.
    // Each level's documentation specifies the incremental set of metrics it
    // enables. As an example, kEnableWait will also enable collecting all the
    // metrics that kEnableCount enables, and its documentation only specifies which
    // extra metrics it also enables.
    //
    // These metrics are identified with some naming conventions, but not all
    // metrics follow exactly this convention. The metrics' own documentation should
    // be source of truth if they diverge.
    enum PerfLevel : unsigned char
    {
        kUninitialized = 0,
        kDisable = 1,
        kEnableCount = 2,
        kEnableWait = 3,
        kEnableTimeExceptForMutex = 4,
        kEnableTimeAndCPUTimeExceptForMutex = 5,
        kEnableTime = 6,
        kOutOfBounds = 7
    };

    void SetPerfLevel(PerfLevel level);

    PerfLevel GetPerfLevel();
}