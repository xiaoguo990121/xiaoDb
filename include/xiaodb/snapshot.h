#pragma once

#include "xiaodb/types.h"

namespace XIAODB_NAMESPACE
{

    class DB;
    // Abstract handle to particular state of a DB.
    // A Snapshot is an immutable object and can therefore be safely
    // accessed from multiple threads without any external synchronization.
    //
    // To Create a Snapshot, call DB::GetSnapshot().
    // To Destroy a Snapshot, call DB::ReleaseSnapshot(snapshot).
    class Snapshot
    {
    public:
        virtual SequenceNumber GetSequenceNumber() const = 0;

        // Returns unix time i.e. the number of seconds since the Epoch, 1970-01-01
        // 00:00:00 (UTC).
        virtual int64_t GetUnixTime() const = 0;

        virtual uint64_t GetTimestamp() const = 0;

    protected:
        virtual ~Snapshot();
    };

    // Simple RAII wrapper class for Snapshot.
    // Constructing this object will create a snapshot.  Destructing will
    // release the snapshot.
    class ManagedSnapshot
    {
    public:
        explicit ManagedSnapshot(DB *db);

        // Instead of creating a snapshot, take ownership of the input snapshot.
        ManagedSnapshot(DB *db, const Snapshot *_snapshot);

        ~ManagedSnapshot();

        const Snapshot *snapshot();

    private:
        DB *db_;
        const Snapshot *snapshot_;
    };
}