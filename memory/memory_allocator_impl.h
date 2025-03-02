#pragma once

#include <algorithm>

#include "xiaodb/memory_allocator.h"

namespace XIAODB_NAMESPACE
{

    struct CustomDeleter
    {
        CustomDeleter(MemoryAllocator *a = nullptr) : allocator(a) {}

        void operator()(char *ptr) const
        {
            if (allocator)
            {
                allocator->Deallocate(ptr);
            }
            else
            {
                delete[] ptr;
            }
        }

        MemoryAllocator *allocator;
    };

    using CacheAllocationPtr = std::unique_ptr<char[], CustomDeleter>;

    inline CacheAllocationPtr AllocateBlock(size_t size,
                                            MemoryAllocator *allocator)
    {
        if (allocator)
        {
            auto block = reinterpret_cast<char *>(allocator->Allocate(size));
            return CacheAllocationPtr(block, allocator);
        }
        return CacheAllocationPtr(new char[size]);
    }

    inline CacheAllocationPtr AllocateAndCopyBlock(const Slice &data,
                                                   MemoryAllocator *allocator)
    {
        CacheAllocationPtr cap = AllocateBlock(data.size(), allocator);
        std::copy_n(data.data(), data.size(), cap.get());
        return cap;
    }
}