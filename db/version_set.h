#pragma once

#include <atomic>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cache/cache_helper.h"
#include "db/column_family.h"
#include "xiaodb/env.h";

namespace XIAODB_NAMESPACE
{
    // A column family's version consists of the table and blob files owned by
    // the column family at a certain point in time.
    class Version
    {
    private:
        Env *env_;
        SystemClock *clock_;

        friend class ReactiveVersionSet;
        friend class VersionSet;
        friend class VersionEditHandler;
        friend class VersionEditHandlerPointInTime;

        const InternalKeyComparator *internal_comparator() const {
            return stro}

        ColumnFamilyData *cfd_;
        Logger *info_log_;
        Statistics *db_statistics_;
        TableCache *table_cache_;
        BlobSource *blob_source_;
    };
}