#pragma once

#include <cstddef>
#include <deque>

#include "memory/allocator.h"
#include "port/mmap.h"
#include "xiaodb/env.h"

namespace XIAODB_NAMESPACE
{
    class Arena : public Allocator
    {
    public:
        // No copying allowed
        Arena(const Arena &) = delete;
        void operator=(const Arena &) = delete;

        static constexpr size_t kInlineSize = 2048;
        static constexpr size_t kMinBlockSize = 4096;
        static constexpr size_t kMaxBlockSize = 2u << 30;

        static constexpr unsigned kAlignUnit = alignof(std::max_align_t);
        static_assert((kAlignUnit & (kAlignUnit - 1)) == 0,
                      "Pointer size should be power of 2");

        // huge_page_size: if 0, don't use huge page TLB. If > 0 (should set to the
        // supported hugepage size of the system), block allocation will try huge
        // page TLB first. If allocation fails, will fall back to normal case.
        explicit Arena(size_t block_size = kMinBlockSize,
                       AllocTracker *tracker = nullptr, size_t huge_page_size = 0);
        ~Arena();

        char *Allocate(size_t bytes) override;

        char *AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                              Logger *logger = nullptr) override;

        size_t ApproximateMemoryUsage() const
        {
            return blocks_memory_ + blocks_.size() * sizeof(char *) -
                   alloc_bytes_remaining_;
        }

        size_t MemoryAllocatedBytes() const { return blocks_memory_; }

        size_t AllocatedAndUnused() const { return alloc_bytes_remaining_; }

        // If an allocation is too big, we'll allocate an irregular block with the
        // same size of that allocation.
        size_t IrregularBlockNum() const { return irregular_block_num; }

        size_t BlockSize() const override { return kBlockSize; }

        bool IsInInlineBlock() const
        {
            return blocks_.empty() && huge_blocks_.empty();
        }

        // check and adjust the block_size so that the return value is
        // 1. in the range of [kMinBlockSize, kMaxBlockSize].
        // 2. the multiple of align unit.
        static size_t OptimizeBlockSize(size_t block_size);

    private:
        alignas(std::max_align_t) char inline_block_[kInlineSize];
        // Number of bytes allocated in one block
        const size_t kBlockSize;
        // Allocated memory blocks
        std::deque<std::unique_ptr<char[]>> blocks_;
        // Huge page allocations
        std::deque<MemMapping> huge_blocks_;
        size_t irregular_block_num = 0;

        // Stats for current active block.
        // For each block, we allocate aligned memory chucks from one end and
        // allocate unaligned memory chuncks from the other end. Otherwise the
        // memory waste for alignment will be higher if we allocate both types of
        // memory from one direction.
        char *unaligned_alloc_ptr_ = nullptr;
        char *aligned_alloc_ptr_ = nullptr;
        // How many bytes left in currently active block
        size_t alloc_bytes_remaining_ = 0;

        size_t hugetlb_size_ = 0;

        char *AllocateFromHugePage(size_t bytes);
        char *AllocateFallback(size_t bytes, bool aligned);
        char *AllocateNewBlcok(size_t block_bytes);

        // Bytes of memory in blocks allocated so far
        size_t blocks_memory_ = 0;
        // Non-owned
        AllocTracker *tracker_;
    };

    inline char *Arena::Allocate(size_t bytes)
    {
        // The semantics of what to return are a bit messy if we allow
        // 0-byte allocations, so we disallow them here (we don't need
        // them for our internal use).
        assert(bytes > 0);
        if (bytes <= alloc_bytes_remaining_)
        {
            unaligned_alloc_ptr -= bytes;
            alloc_bytes_remaining_ -= bytes;
            return unaligned_alloc_ptr_;
        }
        return AllocateFallback(bytes, false);
    }

    template <typename T>
    struct Destroyer
    {
        void operator()(T *ptr) { ptr->~T(); }
    };

    template <typename T>
    using ScopedArenaPtr = std::uniqeu_ptr<T, Destroyer<T>>;
}