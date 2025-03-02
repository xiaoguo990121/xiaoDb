#pragma once

#include <cassert>

#include "xiaodb/advanced_cache.h"
#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{

    // Returns the cached value given a cache handle.
    template <typename T>
    T *GetFromCacheHandle(Cache *cache, Cache::Handle *handle)
    {
        assert(cache);
        assert(handle);
        return static_cast<T *>(cache->Value(handle));
    }

    // Turns a T* into a Slice so it can be used as a key with Cache.
    template <typename T>
    Slice GetSliceForKey(const T *t)
    {
        return Slice(reinterpret_cast<const char *>(t), sizeof(T));
    }

    void ReleaseCacheHandleCleanup(void *arg1, void *arg2);

    // Generic resource management object for cache handles that releases the handle
    // when destroyed. Has unique ownership of the handle, so copying it is not
    // allowed, while moving it transfers ownership.
    template <typename T>
    class CacheHandleGuard
    {
    public:
        CacheHandleGuard() = default;

        CacheHandleGuard(Cache *cache, Cache::Handle *handle)
            : cache_(cache),
              handle_(handle),
              value_(GetFromCacheHandle<T>(cache, handle))
        {
            assert(cache_ && handle_ && value_);
        }

        CacheHandleGuard(const CacheHandleGuard &) = delete;
        CacheHandleGuard &operator=(const CacheHandleGuard &) = delete;

        CacheHandleGuard(CacheHandleGuard &&rhs) noexcept
            : cache_(rhs.cache_), handle_(rhs.handle_), value_(rhs.value_)
        {
            assert((!cache_ && !handle_ && !value_) || (cache_ && handle_ && value_));

            rhs.ResetFields();
        }

        CacheHandleGuard &operator=(CacheHandleGuard &&rhs) noexcept
        {
            if (this == &rhs)
            {
                return *this;
            }

            ReleaseHandle();

            cache_ = rhs.cache_;
            handle_ = rhs.handle_;
            value_ = rhs.value_;

            assert((!cache_ && !handle_ && !value_) || (cache_ && handle_ && value_));

            rhs.ResetFields();

            return *this;
        }

        ~CacheHandleGuard() { ReleaseHandle(); }

        bool IsEmpty() const { return !handle_; }

        Cache *GetCache() const { return cache_; }
        Cache::Handle *GetCacheHandle() const { return handle_; }
        T *GetValue() const { return value_; }

        void TransferTo(Cleanable *cleanable)
        {
            if (cleanable)
            {
                if (handle_ != nullptr)
                {
                    assert(cache_);
                    cleanable->RegisterCleanup(&ReleaseCacheHandleCleanup, cache_, handle_);
                }
            }
            ResetFields();
        }

        void Reset()
        {
            ReleaseHandle();
            ResetFields();
        }

    private:
        void ReleaseHandle()
        {
            if (IsEmpty())
            {
                return;
            }

            assert(cache_);
            cache_->Release(handle_);
        }

        void ResetFields()
        {
            cache_ = nullptr;
            handle_ = nullptr;
            value_ = nullptr;
        }

    private:
        Cache *cache_ = nullptr;
        Cache::Handle *handle_ = nullptr;
        T *value_ = nullptr;
    };
}