#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <list>
#include <mutex>

#include "xiaodb/cache.h"

namespace XIAODB_NAMESPACE
{
    class CacheReservationManager;

    class StallInterface
    {
    public:
        virtual ~StallInterface() {}

        virtual void Block() = 0;

        virtual void Signal() = 0;
    };

    class WriteBufferManager final
    {
    public:
        explicit WriteBufferManager(size_t _buffer_size,
                                    std::shared_ptr<Cache> cache = {},
                                    bool allow_stall = false);

        WriteBufferManager(const WriteBufferManager &) = delete;
        WriteBufferManager &operator=(const WriteBufferManager &) = delete;

        ~WriteBufferManager();

        bool enabled() const { return buffer_size() > 0; }

        bool cost_to_cache() const { return cache_res_mgr_ != nullptr; }

        size_t memory_usage() const
        {
            return memory_used_.load(std::memory_order_relaxed);
        }

        size_t mutable_memtable_memory_usage() const
        {
            return memory_active_.load(std::memory_order_relaxed);
        }

        size_t dummy_entries_in_cache_usage() const;

        size_t buffer_size() const
        {
            return buffer_size_.load(std::memory_order_relaxed);
        }

        void SetBufferSize(size_t new_size)
        {
            assert(new_size > 0);
            buffer_size_.store(new_size, std::memory_order_relaxed);
            mutable_limit_.store(new_size * 7 / 8, std::memory_order_relaxed);

            MaybeEndWriteStall();
        }

        void SetAllowStall(bool new_allow_stall)
        {
            allow_stall_.store(new_allow_stall, std::memory_order_relaxed);
            MaybeEndWriteStall();
        }

        bool ShouldFlush() const
        {
            if (enabled())
            {
                if (mutable_memtable_memory_usage() >
                    mutable_limit_.load(std::memory_order_relaxed))
                {
                    return true;
                }
                size_t local_size = buffer_size();
                if (memory_usage() >= local_size &&
                    mutable_memtable_memory_usage() >= local_size / 2)
                {
                    return true;
                }
            }
            return false;
        }

        bool shouldStall() const
        {
            if (!allow_stall_.load(std::memory_order_relaxed) || !enabled())
            {
                return false;
            }

            return IsStallActive() || IsStallThresholdExceeded();
        }

        bool IsStallActive() const
        {
            return stall_active_.load(std::memory_order_relaxed);
        }

        bool IsStallThresholdExceeded() const
        {
            return memory_usage() >= buffer_size_;
        }

        void ReserveMem(size_t mem);

        void ScheduleFreeMem(size_t mem);

        void FreeMem(size_t mem);

        void BeginWriteStall(StallInterface *wbm_stall);

        void MaybeEndWriteStall();

        void RemoveDBFromQueue(StallInterface *wbm_stall);

    private:
        std::atomic<size_t> buffer_size_;
        std::atomic<size_t> mutable_limit_;
        std::atomic<size_t> memory_used_;

        std::atomic<size_t> memory_active_;
        std::shared_ptr<CacheReservationManager> cache_res_mgr_;

        std::mutex cache_res_mgr_mu_;

        std::list<StallInterface *> queue_;

        std::mutex mu_;
        std::atomic<bool> allow_stall_;

        std::atomic<bool> stall_active_;

        void ReserveMemWithCache(size_t mem);
        void FreeMemWithCache(size_t mem);
    };
}