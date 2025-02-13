#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <memory>
#include <stdexcept>
#include <unordered_set>

#include "xiaodb/customizable.h"
#include "xiaodb/slice.h"

namespace XIAODB_NAMESPACE
{
    class Arena;
    class Allocator;
    class LookupKey;
    class SliceTransform;
    class Logger;
    struct DBOptions;

    using KeyHandle = void *;

    Slice GetLengthPrefixedSlice(const char *data);

    class MemTableRep
    {
    public:
    protected:
        virtual Slice UserKey(const char *key) const;

        Allocator *allocator_;
    };
}