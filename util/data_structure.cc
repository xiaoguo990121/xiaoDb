#include "xiaodb/data_structure.h"

#include "util/math.h"

namespace XIAODB_NAMESPACE::detail
{
    int CountTrailingZeroBitsFromSmallEnumSet(uint64_t v)
    {
        return CountTrailingZeroBits(v);
    }
}