#pragma once

#include <memory>

#include "xiaodb/customizable.h"
#include "xiaodb/status.h"

namespace XIAODB_NAMESPACE
{
    class MemoryAllocator : public Customizable
    {
    public:
        static const char *Type() { return "MemoryAllocator"; }
        static Status CreateFromString(const ConfigOptions &options,
                                       const std::string &value,
                                       std::shared_ptr<MemoryAllocator> *result);

        virtual void *Allocate(size_t size) = 0;

        virtual void Deallocate(void *p) = 0;

        virtual size_t UsableSize(void *, size_t allocation_size) const
        {
            return allocation_size;
        }

        std::string GetId() const override { return GenerateIndividualId(); }
    };

    struct JemallocAllocatorOptions
    {
        static const char *kName() { return "JemallocAllocatorOptions"; }

        bool limit_tcache_size = false;

        size_t tcache_size_lower_bound = 1024;

        size_t tcache_size_upper_bound = 16 * 1024;

        size_t num_arenas = 1;
    };

    Status NewJemllocNodumpAllocator(
        const JemallocAllocatorOptions &options,
        std::shared_ptr<MemoryAllocator> *memory_allocator);
}