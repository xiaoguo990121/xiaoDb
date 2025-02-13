#pragma once

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    namespace port
    {
        class CondVar;
    }

    enum class CpuPriority
    {
        kIdle = 0,
        kLow = 1,
        kNormal = 2,
        kHigh = 3,
    };
}