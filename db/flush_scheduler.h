#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <set>

#include "util/autovector.h"

namespace XIAODB_NAMESPACE
{
    class ColumnFamilyData;

    // FlushScheduler keeps track of all column families whose memtable may
    // be full and require flushing. Unless otherwise noted, all methods on
    // FlushScheduler should be called only with the DB mutex held or from
    // a single-threaded recovery context.
    class FlushScheduler
    {
    public:
        FlushScheduler() : head_(nullptr) {}

        // May be called from multiple threads at once, but not concurrent with
        // any other method calls on this instance
        void ScheduleWork(ColumnFamilyData *cfd);

        // Removes and returns Ref()-ed column family. Client needs to Unref().
        // Filters column families that have been dropped.
        ColumnFamilyData *TakeNextColumnFamily();

        // This can be called concurrently with SchduleWork but it would miss all
        // the scheduled flushes after the last synchronization. This would result
        // into less precise enforcement of memtable sizes but should not matter much.
        bool Empty();

        void Clear();

    private:
        struct Node
        {
            ColumnFamilyData *column_family;
            Node *next;
        };

        std::atomic<Node *> head_;
#ifndef NDEBUG
        std::mutex checking_mutex_;
        std::set<ColumnFamilyData *> checking_set_;
#endif
    };
}