#pragma once

#include <stdint.h>

#include <memory>
#include <unordered_map>

#include "xiaodb/slice.h"

namespace XIAODB_NAMESPACE
{
    using ColumnFamilyId = uint32_t;

    using SequenceNumber = uint64_t;

    struct TableProperties;
    using TablePropertiesCollection =
        std::unordered_map<std::string, std::shared_ptr<const TableProperties>>;

    const SequenceNumber kMinUnCommittedSeq = 1;

    enum class TableFileCreationReason
    {
        kFlush,
        kCompaction,
        kRecovery,
        kMisc,
    };

    enum class BlobFileCreationReason
    {
        kFlush,
        kCompaction,
        kRecovery,
    };

    enum FileType
    {
        kWalFile,
        kDBLockFile,
        kTableFile,
        kDescriptorFile,
        kCurrentFile,
        kTempFile,
        kInfoLogFile,
        kMetaDatabase,
        kIdentityFile,
        kOptionsFile,
        kBlobFile
    };

    enum EntryType
    {
        kEntryPut,
        kEntryDelete,
        kEntrySingleDelete,
        kEntryMerge,
        kEntryRangeDeletion,
        kEntryBlobIndex,
        kEntryDeleteWithTimestamp,
        kEntryWideColumnEntity,
        kEntryTimedPut,
        kEntryOther,
    };

    struct ParsedEntryInfo
    {
        Slice user_key;
        Slice timestamp;
        SequenceNumber sequence;
        EntryType type;
    };

    enum class WriteStallCause
    {
        kMemtableLimit,
        kL0FileCountLimit,
        kPendingCompactionBytes,
        kCFScoperWriteStallCauseEnumMax,

        kWrtieBufferManagerLimit,
        kDBScopeWriteStallCauseEnumMax,
        kNone,
    };

    enum class WriteStallCondition
    {
        kDelayed,
        kStopped,
        kNormal,
    };

    enum class Temperature : uint8_t
    {
        kUnknown = 0,
        kHot = 0x04,
        kWarm = 0x08,
        kCold = 0x0C,
        kLastTemperature,
    };
}