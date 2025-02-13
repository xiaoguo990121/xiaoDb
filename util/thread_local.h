#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "port/port.h"
#include "util/autovector.h"

namespace XIAODB_NAMESPACE
{
    using UnrefHandler = void (*)(void *ptr);

    class ThreadLocalPtr
    {
    public:
        explicit ThreadLocalPtr(UnrefHandler handler = nullptr);

        ThreadLocalPtr(const ThreadLocalPtr &) = delete;
        ThreadLocalPtr &operator=(const ThreadLocalPtr &) = delete;

        ~ThreadLocalPtr();

        void *Get() const;

        void Reset(void *ptr);

        void *Swap(void *ptr);

        bool CompareAndSwap(void *ptr, void *&expected);

        void Scrape(autovector<void *> *ptrs, void *const replacement);

        using FoldFunc = std::function<void(void *, void *)>;

        void Fold(FoldFunc func, void *res);

        static uint32_t TEST_PeekId();

        static void InitSingletons();

        class StaticMeta;

    private:
        static StaticMeta *Instance();

        const uint32_t id_;
    };
}