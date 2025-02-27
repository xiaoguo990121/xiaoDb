#include <cassert>

#include "memory/allocator.h"
#include "memory/arena.h"
#include "xiaodb/write_buffer_manager.h"

namespace XIAODB_NAMESPACE
{

    AllocTracker::AllocTracker(WriteBufferManager *write_buffer_manager)
        : write_buffer_manager_(write_buffer_manager),
          bytes_allocated_(0),
          done_allocating_(false),
          freed_(false) {}

    AllocTracker::~AllocTracker() { FreeMem(); }

    void AllocTracker::Allocate(size_t bytes)
    {
        assert(write_buffer_manager_ != nullptr);
        if (write_buffer_manager_->enabled() ||
            write_buffer_manager_->cost_to_cache())
        {
            bytes_allocated_.fetch_add(bytes, std::memory_order_relaxed);
            write_buffer_manager_->ReserveMem(bytes);
        }
    }

    void AllocTracker::DoneAllocating()
    {
        if (write_buffer_manager_ != nullptr && !done_allocating_)
        {
            if (write_buffer_manager_->enabled() ||
                write_buffer_manager_->cost_to_cache())
            {
                write_buffer_manager_->ScheduleFreeMem(
                    bytes_allocated_.load(std::memory_order_relaxed));
            }
            else
            {
                assert(bytes_allocated_.load(std::memory_order_relaxed) == 0);
            }
            done_allocating_ = true;
        }
    }

    void AllocTracker::FreeMem()
    {
        if (!done_allocating_)
        {
            DoneAllocating();
        }
        if (write_buffer_manager_ != nullptr && !freed_)
        {
            if (write_buffer_manager_->enabled() ||
                write_buffer_manager_->cost_to_cache())
            {
                write_buffer_manager_->FreeMem(
                    bytes_allocated_.load(std::memory_order_relaxed));
            }
            else
            {
                assert(bytes_allocated_.load(std::memory_order_relaxed) == 0);
            }
            freed_ = true;
        }
    }
}