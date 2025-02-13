#pragma once

#include <cassert>
#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

#include "port/likely.h"
#include "port/port.h"
#include "util/math.h"
#include "util/random.h"

namespace XIAODB_NAMESPACE
{
    // An array of core-local values. Ideally the value type, T, is cache aligned to
    // prevent false sharing.
    template <typename T>
    class CoreLocalArray
    {
    public:
        CoreLocalArray();

        size_t Size() const;

        T *Access() const;

        std::pair<T *, size_t> AccessElementAndIndex() const;

        T *AccessAtCore(size_t core_idx) const;

    private:
        std::unique_ptr<T[]> data_;
        int size_shift_;
    };

    template <typename T>
    CoreLocalArray<T>::CoreLocalArray()
    {
        int num_cpus = static_cast<int>(std::thread::hardware_concurrency());

        size_shift_ = 3;
        while (1 << size_shift_ < num_cpus)
        {
            ++size_shift_;
        }
        data_.reset(new T[static_cast<size_t>(1) << size_shift_]);
    }

    template <typename T>
    size_t CoreLocalArray<T>::Size() const
    {
        return static_cast<size_t>(1) << size_shift_;
    }

    template <typename T>
    T *CoreLocalArray<T>::Access() const
    {
        return AccessElementAndIndex().first;
    }

    template <typename T>
    std::pair<T *, size_t> CoreLocalArray<T>::AccessElementAndIndex() const
    {
        int cpuid = port::PhysicalCoreID();
        size_t core_idx;
        if (UNLIKELY(cpuid < 0))
        {
            core_idx = Random::GetTLSInstance()->Uniform(1 << size_shift_);
        }
        else
        {
            core_idx = static_cast<size_t>(BottomNBits(cpuid, size_shift_));
        }
        return {AccessAtCore(core_idx), core_idx};
    }

    template <typename T>
    T *CoreLocalArray<T>::AccessAtCore(size_t core_idx) const
    {
        assert(core_idx < static_cast<size_t>(1) << size_shift_);
        return &data_[core_idx];
    }
}