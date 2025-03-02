#pragma once

#include "xiaodb/xiaodb_namespace.h"

// This file declares a wrapper around the efficient folly DistributedMutex
// that falls back on a standard mutex when not available. See
// https://github.com/facebook/folly/blob/main/folly/synchronization/DistributedMutex.h
// for benefits and limitations.

// At the moment, only scoped locking is supported using DMutexLock
// RAII wrapper, because lock/unlock APIs will vary.

#ifdef USE_FOLLY

#include <folly/synchronization/DistributedMutex.h>

namespace XIAODB_NAMESPACE
{

    class DMutex : public folly::DistributedMutex
    {
    public:
        static const char *kName() { return "folly::DistributedMutex"; }

        explicit DMutex(bool IGNORED_adaptive = false) { (void)IGNORED_adaptive; }

        // currently no-op
        void AssertHeld() const {}
    };
    using DMutexLock = std::lock_guard<folly::DistributedMutex>;

} // namespace ROCKSDB_NAMESPACE

#else

#include <mutex>

#include "port/port.h"

namespace XIAODB_NAMESPACE
{

    using DMutex = port::Mutex;
    using DMutexLock = std::lock_guard<DMutex>;

} // namespace ROCKSDB_NAMESPACE
#endif