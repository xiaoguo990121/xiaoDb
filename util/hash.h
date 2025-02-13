#pragma once

#include <cstddef>
#include <cstdint>

#include "xiaodb/slice.h"
#include "util/fastrange.h"

namespace XIAODB_NAMESPACE
{
    // Stable/persistent 64-bit hash. Higher quality and generally faster than
    // Hash(), especially for inputs > 24 bytes.
    // KNOWN FLAW: incrementing seed by 1 might not give sufficiently independent
    // results from previous seed. Recommend incrementing by a large odd number.
    uint64_t Hash64(const char *data, size_t n, uint64_t seed);

    // Specific optimization without seed (same as seed = 0)
    uint64_t Hash64(const char *data, size_t n);

    inline uint64_t NPHash64(const char *data, size_t n, uint64_t seed)
    {
#ifdef XIAODB_MODIFY_NPHAHS
        return Hash64(data, n, seed + 123456789);
#else
        return Hash64(data, n, seed);
#endif
    }

    inline uint64_t NPHash64(const char *data, size_t n)
    {
#ifdef XIAODB_MODIFY_NPHAHS
        // For testing "subject to change"
        return Hash64(data, n, 123456789);
#else
        // Currently same as Hash64
        return Hash64(data, n);
#endif
    }

    void Hash2x64(const char *data, size_t n, uint64_t *high64, uint64_t *low64);
    void Hash2x64(const char *data, size_t n, uint64_t seed, uint64_t *high64,
                  uint64_t *low64);

    void BijectiveHash2x64(uint64_t in_high64, uint64_t in_low64,
                           uint64_t *out_high64, uint64_t *out_low64);
    void BijectiveHash2x64(uint64_t in_high64, uint64_t in_low64, uint64_t seed,
                           uint64_t *out_high64, uint64_t *out_low64);

    void BijectiveUnhash2x64(uint64_t in_high64, uint64_t in_low64,
                             uint64_t *out_high64, uint64_t *out_low64);
    void BijectiveUnhash2x64(uint64_t in_high64, uint64_t in_low64, uint64_t seed,
                             uint64_t *out_high64, uint64_t *out_low64);

    uint32_t Hash32(const char *data, size_t n, uint32_t seed);

    inline uint32_t BloomHash32(const Slice &key)
    {
        return Hash32(key.data(), key.size(), 0xbc9f1d34);
    }

    inline uint64_t GetSliceHash64(const Slice &key)
    {
        return Hash64(key.data(), key.size());
    }
    // Provided for convenience for use with template argument deduction, where a
    // specific overload needs to be used.
    extern uint64_t (*kGetSliceNPHash64UnseededFnPtr)(const Slice &);

    inline uint64_t GetSliceNPHash64(const Slice &s)
    {
        return NPHash64(s.data(), s.size());
    }

    inline uint64_t GetSliceNPHash64(const Slice &s, uint64_t seed)
    {
        return NPHash64(s.data(), s.size(), seed);
    }

    // Similar to `GetSliceNPHash64()` with `seed`, but input comes from
    // concatenation of `Slice`s in `data`.
    uint64_t GetSlicePartsNPHash64(const SliceParts &data, uint64_t seed);

    inline size_t GetSliceRangedNPHash(const Slice &s, size_t range)
    {
        return FastRange64(NPHash64(s.data(), s.size()), range);
    }

    // TODO: consider rename to GetSliceHash32
    inline uint32_t GetSliceHash(const Slice &s)
    {
        return Hash32(s.data(), s.size(), 397);
    }

    // Useful for splitting up a 64-bit hash
    inline uint32_t Upper32of64(uint64_t v)
    {
        return static_cast<uint32_t>(v >> 32);
    }
    inline uint32_t Lower32of64(uint64_t v) { return static_cast<uint32_t>(v); }

    // std::hash-like interface.
    struct SliceHasher32
    {
        uint32_t operator()(const Slice &s) const { return GetSliceHash(s); }
    };
    struct SliceNPHasher64
    {
        uint64_t operator()(const Slice &s, uint64_t seed = 0) const
        {
            return GetSliceNPHash64(s, seed);
        }
    };
}