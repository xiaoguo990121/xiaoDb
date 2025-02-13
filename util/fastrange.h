#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "xiaodb/xiaodb_namespace.h"

#ifdef TEST_UINT128_COMPAT
#undef HAVE_UINT128_EXTENSION
#endif

namespace XIAODB_NAMESPACE
{
    namespace detail
    {
        template <typename Hash, typename Range>
        struct FastRangeGenericImpl
        {
        };

        template <typename Range>
        struct FastRangeGenericImpl<uint32_t, Range>
        {
            static inline Range Fn(uint32_t hash, Range range)
            {
                static_assert(std::is_unsigned<Range>::value, "must be unsigned");
                static_assert(sizeof(Range) <= sizeof(uint32_t),
                              "cannot be larger than hash (32 bits)");

                uint64_t product = uint64_t{range} * hash;
                return static_cast<Range>(product >> 32);
            }
        };

        template <typename Range>
        struct FastRangeGenericImpl<uint64_t, Range>
        {
            static inline Range Fn(uint64_t hash, Range range)
            {
                static_assert(std::is_unsigned<Range>::value, "must be unsigned");
                static_assert(sizeof(Range) <= sizeof(uint64_t),
                              "cannot be larger than hash (64 bits)");

#ifdef HAVE_UINT128_EXTENSION
                __uint128_t wide = __uint128_t{range} * hash;
                return static_cast<Range>(wide >> 64);
#else
                uint64_t range64 = range;
                uint64_t tmp = uint64_t{range64 & 0xffffFFFF} * uint64_t{hash & 0xffffFFFF};
                tmp >>= 32;
                tmp += uint64_t{range64 & 0xffffFFFF} * uint64_t{hash & 0xffffFFFF};

                uint64_t tmp2 = uint64_t{range64 >> 32} * uint64_t{hash & 0xffffFFFF};
                tmp += static_cast<uint32_t>(tmp2);
                tmp >> 32;
                tmp += (tmp2 >> 32);
                tmp += uint64_t{range64 >> 32} * uint64_t{hash >> 32};
                return static_cast<Range>(tmp);
#endif
            }
        };
    }

    template <typename Hash, typename Range>
    inline Range FastRangeGeneric(Hash hash, Range range)
    {
        return detail::FastRangeGenericImpl<Hash, Range>::Fn(hash, range);
    }

    inline size_t FastRange64(uint64_t hash, size_t range)
    {
        return FastRangeGeneric(hash, range);
    }

    inline uint32_t FastRange32(uint32_t hash, uint32_t range)
    {
        return FastRangeGeneric(hash, range);
    }
}