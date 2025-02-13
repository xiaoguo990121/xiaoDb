#pragma once

#include "monitoring/perf_level_imp.h"
#include "xiaodb/comparator.h"

namespace XIAODB_NAMESPACE
{

    class UserComparatorWrapper
    {
    public:
        UserComparatorWrapper() : user_comparator_(nullptr) {}

        explicit UserComparatorWrapper(const Comparator *const user_cmp)
            : user_comparator_(user_cmp) {}

        ~UserComparatorWrapper() = default;

        const Comparator *user_comparator() const { return user_comparator_; }

    private:
        const Comparator *user_comparator_;
    };
}