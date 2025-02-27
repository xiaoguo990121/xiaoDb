#pragma once

#include <stdint.h>
#include <stdio.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "xiaodb/attribute_groups.h"

#if defined(__GNUC__) || defined(__clang__)
#define XIAODB_DEPRECATED_FUNC __attribute__((__deprecated__))
#elif _WIN32
#define XIAODB_DEPRECATED_FUNC __declspec(deprecated)
#endif

namespace XIAODB_NAMESPACE
{

    struct ColumnFamilyOptions;
    struct CompactionOptions;
    struct CompactRangeOptions;
    struct DBOptions;
    struct ExternalSstFileInfo;
    struct FlushOptions;
    struct ReadOptions;
    struct TableProperties;
    struct WriteOptions;
    struct WatiForCompactOptions;
    class Env;
    class EventListener;
    class FileSystem;
    class Replayer;
    class StatsHistoryIterator;
    class TraceReader;
    class TraceWriter;
    class WriteBatch;

    extern const std::string kDefaultColumnFamilyName;
    extern const std::string kPersistentStatsColumnFamilyName;
    struct ColumnFamilyDescriptor
    {
        std::string name;
        ColumnFamilyDescriptor()
            : name(kDefaultColumnFamilyName), options(ColumnFamilyOptions()) {}
        ColumnFamilyDescriptor(const std::string &_name,
                               const ColumnFamilyOptions &_options)
            : name(_name), options(_options) {}
    };

    class ColumnFamilyHandle
    {
    public:
        virtual ~ColumnFamilyHandle() {}
        // Returns the name of the column family associated with the current handle.
        virtual const std::string &GetName() const = 0;
        // Returns the ID of the column family associated with the current handle.
        virtual uint32_t GetID() const = 0;
        // Fills "*desc" with the up-to-date descriptor of the column family
        // associated with this handle. Since it fills "*desc" with the up-to-date
        // information, this call might internally lock and release DB mutex to
        // access the up-to-date CF options.  In addition, all the pointer-typed
        // options cannot be referenced any longer than the original options exist.
        //
        // Note that this function is not supported in RocksDBLite.
        virtual Status GetDescriptor(ColumnFamilyDescriptor *desc) = 0;
        // Returns the comparator of the column family associated with the
        // current handle.
        virtual const Comparator *GetComparator() const = 0;
    };

    struct GetMergeOperandsOptions
    {
        using ContinueCallback = std::function<bool(Slice)>;

        // A limit on the number of merge operands returned by the GetMergeOperands()
        // API. In contrast with ReadOptions::merge_operator_max_count, this is a hard
        // limit: when it is exceeded, no merge operands will be returned and the
        // query will fail with an Incomplete status. See also the
        // DB::GetMergeOperands() API below.
        int expected_max_number_of_operands = 0;

        // `continue_cb` will be called after reading each merge operand, excluding
        // any base value. Operands are read in order from newest to oldest. The
        // operand value is provided as an argument.
        //
        // Returning false will end the lookup process at the merge operand on which
        // `continue_cb` was just invoked. Returning true allows the lookup to
        // continue.
        //
        // If it is nullptr, `GetMergeOperands()` will behave as if it always returned
        // true (continue fetching merge operands until there are no more).
        ContinueCallback continue_cb;
    };
}