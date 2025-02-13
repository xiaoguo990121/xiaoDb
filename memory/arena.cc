#include "memory/arena.h"

#include <algorithm>

#include "logging/logging.h"
#include "port/malloc.h"
#include "port/port.h"
#include "xiaodb/env.h"
#include "util/string_util.h"

namespace XIAODB_NAMESPACE
{
    size_t Arena::OptimizeBlockSize(size_t block_size)
    {
        // Make sure block_size is in optimal range
        block_size = std::max(Arena::kMinBlockSize, block_size);
        block_size = std::min(Arena::kMaxBlockSize, block_size);

        // make sure block_size is the multiple of kAlignUnit
        if (block_size % kAlignUnit != 0)
        {
            block_size = (1 + block_size / kAlignUnit) * kAlignUnit;
        }

        return block_size;
    }

    Arena::Arena(size_t block_size, AllocTracker *tracker, size_t huge_page_size)
        : kBlockSize(OptimizeBlockSize(block_size)), tracker_(tracker)
    {
        assert(kBlockSize >= kMinBlockSize && kBlockSize <= kMaxBlockSize &&
               kBlockSize % kAlignUnit == 0);

        alloc_bytes_remaining_ = sizeof(inline_block_);
        blocks_memory_ += alloc_bytes_remaining_;
        aligned_alloc_ptr_ = inline_block_;
        unaligned_alloc_ptr_ = inline_block_ + alloc_bytes_remaining_;
        if (MemMapping::kHugePageSupported)
        {
            hugetlb_size_ = huge_page_size;
            if (hugetlb_size_ && kBlockSize > hugetlb_size_)
            {
                hugetlb_size_ = ((kBlockSize - 1U) / hugetlb_size_ + 1U) * hugetlb_size_;
            }
        }
        if (tracker_ != nullptr)
        {
            tracker_->Allocate(kInlineSize);
        }
    }

    Arena::~Arena()
    {
        if (tracker_ != nullptr)
        {
            assert(tracker_->is_freed());
            tracker_->FreeMem();
        }
    }

    char *Arena::AllocateFallback(size_t bytes, bool aligned)
    {
        if (bytes > kBlockSize / 4)
        {
            ++irregular_block_num;
            return AllocateNewBlcok(bytes);
        }

        size_t size = 0;
        char *block_head = nullptr;
        if (MemMapping::kHugePageSupported && hugetlb_size_ > 0)
        {
            size = hugetlb_size_;
            block_head = AllocateFromHugePage(size);
        }
        if (!block_head)
        {
            size = kBlockSize;
            block_head = AllocateNewBlcok(size);
        }
        alloc_bytes_remaining_ = size - bytes;

        if (aligned)
        {
            aligned_alloc_ptr_ = block_head + bytes;
            unaligned_alloc_ptr_ = block_head + size;
            return block_head;
        }
        else
        {
            aligned_alloc_ptr_ = block_head;
            unaligned_alloc_ptr_ = block_head + size - bytes;
            return unaligned_alloc_ptr_;
        }
    }

    char *Arena::AllocateFromHugePage(size_t bytes)
    {
        MemMapping mm = MemMapping::AllocateHuge(bytes);
        auto addr = static_cast<char *>(mm.Get());
        if (addr)
        {
            huge_blocks_.push_back(std::move(mm));
            blocks_memory_ += bytes;
            if (tracker_ != nullptr)
            {
                tracker_->Allocate(bytes);
            }
        }
        return addr;
    }

    char *Arena::AllocateAligned(size_t bytes, size_t huge_page_size,
                                 Logger *logger)
    {
        if (MemMapping::kHugePageSupported && hugetlb_size_ > 0 &&
            huge_page_size > 0 && bytes > 0)
        {
            // Allocate from a huge page TLB table.
            size_t reserved_size =
                ((bytes - 1U) / huge_page_size + 1U) * huge_page_size;
            assert(reserved_size >= bytes);

            char *addr = AllocateFromHugePage(reserved_size);
            if (addr == nullptr)
            {
                XIAO_LOG_WARN(logger,
                              "AllocateAligned fail to allocate huge TLB pages: %s",
                              errnoStr(errno).c_str());
            }
            else
            {
                return addr;
            }
        }

        size_t current_mod =
            reinterpret_cast<uintptr_t>(aligned_alloc_ptr_) & (kAlignUnit - 1);
        size_t slop = (current_mod == 0 ? 0 : kAlignUnit - current_mod);
        size_t needed = bytes + slop;
        char *result;
        if (needed <= alloc_bytes_remaining_)
        {
            result = aligned_alloc_ptr_ + slop;
            aligned_alloc_ptr_ += needed;
            alloc_bytes_remaining_ -= needed;
        }
        else
        {
            // AllocateFallback always returns aligned memory
            result = AllocateFallback(bytes, true);
        }
        assert((reinterpret_cast<uintptr_t>(result) & (kAlignUnit - 1)) == 0);
        return result;
    }

    char *Arena::AllocateNewBlcok(size_t block_bytes)
    {
        // NOTE: std::make_unique zero-initializes the block so is not appropriate here
        char *block = new char[block_bytes];
        blocks_.push_back(std::unique_ptr<char[]>(block));

        size_t allocate_size;
#ifdef XIAODB_MALLOC_USABLE_SIZE
        allocate_size = malloc_usable_size(block);
#ifndef NDEBUG
        // It's hard to predict what malloc_usable_size() returns.
        // A callback can allow users to change the costed size.
        std::pair<size_t *, size_t *> pair(&allocate_size, &block_bytes);
        TEST_SYNC_POINT_CALLBACK("Arena::AllocateNewBlock:0", &pair);
#endif
#else
        allocated_size = block_bytes;
#endif
        blocks_memory_ += allocate_size;
        if (tracker_ != nullptr)
        {
            tracker_->Allocate(allocate_size);
        }
        return block;
    }
}