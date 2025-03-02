#pragma once

#include "xiaodb/slice.h"
#include "util/math128.h"

namespace XIAODB_NAMESPACE
{

    // Stable/persistent 128-bit hash for non-cryptographic applications.
    Unsigned128 Hash128(const char *data, size_t n, uint64_t seed);

    // Specific optimization without seed (same as seed = 0)
    Unsigned128 Hash128(const char *data, size_t n);

    inline Unsigned128 GetSliceHash128(const Slice &key)
    {
        return Hash128(key.data(), key.size());
    }
}