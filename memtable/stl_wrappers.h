#pragma once

#include <map>
#include <string>

#include "xiaodb/comparator.h"
#include "xiaodb/memtablerep.h"
#include "xiaodb/slice.h"
#include "util/coding.h"

namespace XIAODB_NAMESPACE
{
    namespace stl_wrappers
    {
        class Base
        {
        protected:
            const MemTableRep::KeyComparator &compare_;
            explicit Base(const MemTableRep::KeyComparator &compare)
                : compare_(compare) {}
        };

        struct Compare : private Base
        {
            explicit Compare(const MemTableRep::KeyComparator &compare) : Base(compare) {}
            inline bool operator()(const char *a, const char *b) const
            {
                return compare_(a, b) < 0;
            }
        };
    }
}