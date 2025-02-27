#pragma once

#include <array>
#include <vector>

#include "xiaodb/db.h"
#include "db/kv_checksum.h"
#include "xiaodb/write_batch.h"
#include "util/autovector.h"

namespace XIAODB_NAMESPACE
{

    class MemTable;
    class FlushScheduler;
    class ColumnFamilyData;

    class ColumnFamilyMemTables
    {
    public:
        virtual ~ColumnFamilyMemTables() {}
        virtual bool Seek(uint32_t column_family_id) = 0;
        // returns true if the update to memtable should be ignored
        // (useful when recovering from log whose updates have already
        // been processed)
        virtual uint64_t GetLogNumber() const = 0;
        virtual MemTable *GetMemTable() const = 0;
        virtual ColumnFamilyHandle *GetColumnFamilyHandle() = 0;
        virtual ColumnFamilyData *current() { return nullptr; }
    };

    class ColumnFamilyMemTableDefault : public ColumnFamilyMemTables
    {
    public:
        explicit ColumnFamilyMemTableDefault(MemTable *mem)
            : ok_(false), mem_(mem) {}

        bool Seek(uint32_t column_family_id) override
        {
            ok_ = (column_family_id == 0);
            return ok_;
        }

        uint64_t GetLogNumber() const override { return 0; }

        MemTable *GetMemTable() const override
        {
            assert(ok_);
            return mem_;
        }

        ColumnFamilyHandle *GetColumnFamilyHandle() override { return nullptr; }

    private:
        bool ok_;
        MemTable *mem_;
    };

    struct WriteBatch::ProtectionInfo
    {
        // `WriteBatch` usually doesn't contain a huge number of keys so protecting
        // with a fixed, non-configurable eight bytes per key may work well enough.
        autovector<ProtectionInfoKVOC64> entries_;

        size_t GetBytesPerKey() const { return 8; }
    };

    // WriteBatchInternal provides static methods for manipulating a
    // WriteBatch that we don't want in the public WriteBatch interface.
    class WriteBatchInternal
    {
    public:
        // WriteBatch header has an 8-byte sequence number followed by a 4-byte count.
        static constexpr size_t kHeader = 12;

        // WriteBatch methods with column_family_id instead of ColumnFamilyHandle*
        static Status Put(WriteBatch *batch, uint32_t column_family_id,
                          const Slice &key, const Slice &value);

        static Status Put(WriteBatch *batch, uint32_t column_family_id,
                          const SliceParts &key, const SliceParts &value);

        static Status TimedPut(WriteBatch *batch, uint32_t column_family_id,
                               const Slice &key, const Slice &value,
                               uint64_t unix_write_time);

        static Status PutEntity(WriteBatch *batch, uint32_t column_family_id,
                                const Slice &key, const WideColumns &columns);

        static Status Delete(WriteBatch *batch, uint32_t column_family_id,
                             const SliceParts &key);

        static Status Delete(WriteBatch *batch, uint32_t column_family_id,
                             const Slice &key);

        static Status SingleDelete(WriteBatch *batch, uint32_t column_family_id,
                                   const SliceParts &key);

        static Status SingleDelete(WriteBatch *batch, uint32_t column_family_id,
                                   const Slice &key);

        static Status DeleteRange(WriteBatch *b, uint32_t column_family_id,
                                  const Slice &begin_key, const Slice &end_key);

        static Status DeleteRange(WriteBatch *b, uint32_t column_family_id,
                                  const SliceParts &begin_key,
                                  const SliceParts &end_key);

        static Status Merge(WriteBatch *batch, uint32_t column_family_id,
                            const Slice &key, const Slice &value);

        static Status Merge(WriteBatch *batch, uint32_t column_family_id,
                            const SliceParts &key, const SliceParts &value);

        static Status PutBlobIndex(WriteBatch *batch, uint32_t column_family_id,
                                   const Slice &key, const Slice &value);

        static ValueType GetBeginPrepareType(bool write_after_commit,
                                             bool unprepared_batch);

        static Status InsertBeginPrepare(WriteBatch *batch,
                                         const bool write_after_commit = true,
                                         bool unprepared_batch = false);

        static Status InsertEndPrepare(WriteBatch *batch, const Slice &xid);

        static Status MarkEndPrepare(WriteBatch *batch, const Slice &xid,
                                     const bool write_after_commit = true,
                                     const bool unprepared_batch = false);

        static Status MarkRollback(WriteBatch *batch, const Slice &xid);

        static Status MarkCommit(WriteBatch *batch, const Slice &xid);

        static Status MarkCommitWithTimestamp(WriteBatch *batch, const Slice &xid,
                                              const Slice &commit_ts);

        static Status InsertNoop(WriteBatch *batch);

        // Return the number of entries in the batch.
        static uint32_t Count(const WriteBatch *batch);

        // Set the count for the number of entries in the batch.
        static void SetCount(WriteBatch *batch, uint32_t n);

        // Return the sequence number for the start of this batch.
        static SequenceNumber Sequence(const WriteBatch *batch);

        // Store the specified number as the sequence number for the start of
        // this batch.
        static void SetSequence(WriteBatch *batch, SequenceNumber seq);

        // Returns the offset of the first entry in the batch.
        // This offset is only valid if the batch is not empty.
        static size_t GetFirstOffset(WriteBatch *batch);

        static Slice Contents(const WriteBatch *batch) { return Slice(batch->rep_); }

        static size_t ByteSize(const WriteBatch *batch) { return batch->rep_.size(); }

        static Status SetContents(WriteBatch *batch, const Slice &contents);

        static Status CheckSlicePartsLength(const SliceParts &key,
                                            const SliceParts &value);

        // Inserts batches[i] into memtable, for i in 0..num_batches-1 inclusive.
        //
        // If ignore_missing_column_families == true. WriteBatch
        // referencing non-existing column family will be ignored.
        // If ignore_missing_column_families == false, processing of the
        // batches will be stopped if a reference is found to a non-existing
        // column family and InvalidArgument() will be returned.  The writes
        // in batches may be only partially applied at that point.
        //
        // If log_number is non-zero, the memtable will be updated only if
        // memtables->GetLogNumber() >= log_number.
        //
        // If flush_scheduler is non-null, it will be invoked if the memtable
        // should be flushed.
        //
        // Under concurrent use, the caller is responsible for making sure that
        // the memtables object itself is thread-local.
        static Status InsertInto(
            WriteThread::WriteGroup &write_group, SequenceNumber sequence,
            ColumnFamilyMemTables *memtables, FlushScheduler *flush_scheduler,
            TrimHistoryScheduler *trim_history_scheduler,
            bool ignore_missing_column_families = false, uint64_t log_number = 0,
            DB *db = nullptr, bool concurrent_memtable_writes = false,
            bool seq_per_batch = false, bool batch_per_txn = true);

        // Convenience form of InsertInto when you have only one batch
        // next_seq returns the seq after last sequence number used in MemTable insert
        static Status InsertInto(
            const WriteBatch *batch, ColumnFamilyMemTables *memtables,
            FlushScheduler *flush_scheduler,
            TrimHistoryScheduler *trim_history_scheduler,
            bool ignore_missing_column_families = false, uint64_t log_number = 0,
            DB *db = nullptr, bool concurrent_memtable_writes = false,
            SequenceNumber *next_seq = nullptr, bool *has_valid_writes = nullptr,
            bool seq_per_batch = false, bool batch_per_txn = true);

        static Status InsertInto(WriteThread::Writer *writer, SequenceNumber sequence,
                                 ColumnFamilyMemTables *memtables,
                                 FlushScheduler *flush_scheduler,
                                 TrimHistoryScheduler *trim_history_scheduler,
                                 bool ignore_missing_column_families = false,
                                 uint64_t log_number = 0, DB *db = nullptr,
                                 bool concurrent_memtable_writes = false,
                                 bool seq_per_batch = false, size_t batch_cnt = 0,
                                 bool batch_per_txn = true,
                                 bool hint_per_batch = false);

        // Appends src write batch to dst write batch and updates count in dst
        // write batch. Returns OK if the append is successful. Checks number of
        // checksum against count in dst and src write batches, and returns Corruption
        // if the count is inconsistent.
        static Status Append(WriteBatch *dst, const WriteBatch *src,
                             const bool WAL_only = false);

        // Returns the byte size of appending a WriteBatch with ByteSize
        // leftByteSize and a WriteBatch with ByteSize rightByteSize
        static size_t AppendedByteSize(size_t leftByteSize, size_t rightByteSize);

        // Iterate over [begin, end) range of a write batch
        static Status Iterate(const WriteBatch *wb, WriteBatch::Handler *handler,
                              size_t begin, size_t end);

        // This write batch includes the latest state that should be persisted. Such
        // state meant to be used only during recovery.
        static void SetAsLatestPersistentState(WriteBatch *b);
        static bool IsLatestPersistentState(const WriteBatch *b);

        static void SetDefaultColumnFamilyTimestampSize(WriteBatch *wb,
                                                        size_t default_cf_ts_sz);

        static std::tuple<Status, uint32_t, size_t> GetColumnFamilyIdAndTimestampSize(
            WriteBatch *b, ColumnFamilyHandle *column_family);

        static bool TimestampsUpdateNeeded(const WriteBatch &wb)
        {
            return wb.needs_in_place_update_ts_;
        }

        static bool HasKeyWithTimestamp(const WriteBatch &wb)
        {
            return wb.has_key_with_ts_;
        }

        // Update per-key value protection information on this write batch.
        // If checksum is provided, the batch content is verfied against the checksum.
        static Status UpdateProtectionInfo(WriteBatch *wb, size_t bytes_per_key,
                                           uint64_t *checksum = nullptr);
    };
}