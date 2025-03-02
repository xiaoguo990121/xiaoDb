#pragma once

#include <memory>

#include "memory/memory_allocator_impl.h"
#include "xiaodb/advanced_cache.h"
#include "xiaodb/xiaodb_namespace.h"
#include "xiaodb/slice.h"
#include "xiaodb/status.h"

namespace XIAODB_NAMESPACE
{
    // A class representing a single uncompressed value read from a blob file.
    class BlobContents
    {
    public:
        BlobContents(CacheAllocationPtr &&allocation, size_t size)
            : allocation_(std::move(allocation)), data_(allocation_.get(), size) {}

        BlobContents(const BlobContents &) = delete;
        BlobContents &operator=(const BlobContents &) = delete;

        BlobContents(BlobContents &&) = default;
        BlobContents &operator=(BlobContents &&) = default;

        ~BlobContents() = default;

        const Slice &data() const { return data_; }
        size_t size() const { return data_.size(); }

        size_t ApproximateMemoryUsage() const;

        // For TypedCacheInterface
        const Slice &ContentSlice() const { return data_; }
        static constexpr CacheEntryRole kCacheEntryRole = CacheEntryRole::kBlobValue;

    private:
        CacheAllocationPtr allocation_;
        Slice data_;
    };

    class BlobContentsCreator : public Cache::CreateContext
    {
    public:
        static void Create(std::unique_ptr<BlobContents> *out, size_t *out_charge,
                           const Slice &contents, CompressionType /*type*/,
                           MemoryAllocator *alloc)
        {
            auto raw = new BlobContents(AllocateAndCopyBlock(contents, alloc),
                                        contents.size());
            out->reset(raw);
            if (out_charge)
            {
                *out_charge = raw->ApproximateMemoryUsage();
            }
        }
    };
}