#pragma once
#include "xiaodb/types.h"

namespace XIAODB_NAMESPACE
{

    enum class SnapshotCheckerResult : int
    {
        kInSnapshot = 0,
        kNotInSnapshot = 1,
        // In case snapshot is released and the checker has no clue whether
        // the given sequence is visible to the snapshot.
        kSnapshotReleased = 2,
    };

    // Callback class that control GC of duplicate keys in flush/compaction.
    class SnapshotChecker
    {
    public:
        virtual ~SnapshotChecker() {}
        virtual SnapshotCheckerResult CheckInSnapshot(
            SequenceNumber sequence, SequenceNumber snapshot_sequence) const = 0;
    };

    class DisableGCSnapshotChecker : public SnapshotChecker
    {
    public:
        virtual ~DisableGCSnapshotChecker() {}
        SnapshotCheckerResult CheckInSnapshot(
            SequenceNumber /*sequence*/,
            SequenceNumber /*snapshot_sequence*/) const override
        {
            // By returning kNotInSnapshot, we prevent all the values from being GCed
            return SnapshotCheckerResult::kNotInSnapshot;
        }
        static DisableGCSnapshotChecker *Instance();

    protected:
        explicit DisableGCSnapshotChecker() {}
    };

    class WritePreparedTxnDB;

    // Callback class created by WritePreparedTxnDB to check if a key
    // is visible by a snapshot.
    class WritePreparedSnapshotChecker : public SnapshotChecker
    {
    public:
        explicit WritePreparedSnapshotChecker(WritePreparedTxnDB *txn_db);
        virtual ~WritePreparedSnapshotChecker() {}

        SnapshotCheckerResult CheckInSnapshot(
            SequenceNumber sequence, SequenceNumber snapshot_sequence) const override;

    private:
        const WritePreparedTxnDB *const txn_db_;
    };

    bool DataIsDefinitelyInSnapshot(SequenceNumber seqno, SequenceNumber snapshot,
                                    const SnapshotChecker *snapshot_checker);

    bool DataIsDefinitelyNotInSnapshot(SequenceNumber seqno,
                                       SequenceNumber snapshot,
                                       const SnapshotChecker *snapshot_checker);
}