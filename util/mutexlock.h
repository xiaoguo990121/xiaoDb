#pragma once

#include <assert.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "port/port.h"
#include "util/fastrange.h"
#include "util/hash.h"

namespace XIAODB_NAMESPACE
{
    // Helper class that locks a mutex on construction and unlocks the mutex when
    // the destructor of the MutexLock object is invoked.
    //
    // Typical usage:
    //
    //   void MyClass::MyMethod() {
    //     MutexLock l(&mu_);       // mu_ is an instance variable
    //     ... some complex code, possibly with multiple return paths ...
    //   }

    class MutexLock
    {
    public:
        explicit MutexLock(port::Mutex *mu) : mu_(mu) { this->mu_->Lock(); }

        MutexLock(const MutexLock &) = delete;
        void operator=(const MutexLock &) = delete;

        ~MutexLock() { this->mu_->Unlock(); }

    private:
        port::Mutex *const mu_;
    };

    class ReadLock
    {
    public:
        explicit ReadLock(port::RWMutex *mu) : mu_(mu) { this->mu_->ReadLock(); }
        // No copying allowed
        ReadLock(const ReadLock &) = delete;
        void operator=(const ReadLock &) = delete;

        ~ReadLock() { this->mu_->ReadUnlock(); }

    private:
        port::RWMutex *const mu_;
    };

    class ReadUnlock
    {
    public:
        explicit ReadUnlock(port::RWMutex *mu) : mu_(mu) { mu->AssertHeld(); }
        // No copying allowed
        ReadUnlock(const ReadUnlock &) = delete;
        ReadUnlock &operator=(const ReadUnlock &) = delete;

        ~ReadUnlock() { mu_->ReadUnlock(); }

    private:
        port::RWMutex *const mu_;
    };

    class WriteLock
    {
    public:
        explicit WriteLock(port::RWMutex *mu) : mu_(mu) { this->mu_->WriteLock(); }
        // No copying allowed
        WriteLock(const WriteLock &) = delete;
        void operator=(const WriteLock &) = delete;

        ~WriteLock() { this->mu_->WriteUnlock(); }

    private:
        port::RWMutex *const mu_;
    };

    class SpinMutex
    {
    public:
        SpinMutex() : locked_(false) {}

        bool try_lock()
        {
            auto currently_locked = locked_.load(std::memory_order_relaxed);
            return !currently_locked &&
                   locked_.compare_exchange_weak(currently_locked, true,
                                                 std::memory_order_acquire,
                                                 std::memory_order_relaxed);
        }

        void lock()
        {
            for (size_t tries = 0;; ++tries)
            {
                if (try_lock)
                {
                    break;
                }
                port::AsmVolatilePause();
                if (tries > 100)
                {
                    std::this_thread::yield();
                }
            }
        }

        void unlock() { locked_.store(false, std::memory_order_release); }

    private:
        std::atomic<bool> locked_;
    };

    template <class T>
    struct ALIGN_AS(CACHE_LINE_SIZE) CacheAlignedWrapper
    {
        T obj;
    };
    template <class T>
    struct UnWrap
    {
        using type = T;
        static type &Go(T &t) { return t; }
    };
    template <class T>
    struct UnWrap<CacheAlignedWrapper<T>>
    {
        using type = T;
        static type &Go(CacheAlignedWrapper<T> &t) { return t.obj_; }
    };

    template <class T, class Key = Slice, class Hash = SliceNPHasher64>
    class Striped
    {
    public:
        explicit Striped(size_t stripe_count)
            : stripe_count_(stripe_count), data_(new T[stripe_count]) {}

        using Unwrapped = typename Unwrap<T>::type;
        Unwrapped &Get(const Key &key, uint64_t seed = 0)
        {
            size_t index = FastRangeGeneric(hash_(key, seed), stripe_count_);
            return Unwrap<T>::Go(data_[index]);
        }

        size_t ApproximateMemoryUsage() const
        {
            // NOTE: could use malloc_usable_size() here, but that could count unmapped
            // pages and could mess up unit test OccLockBucketsTest::CacheAligned
            return sizeof(*this) + stripe_count_ * sizeof(T);
        }

    private:
        size_t stripe_count_;
        std::unique_ptr<T[]> data_;
        Hash hash_;
    };
}