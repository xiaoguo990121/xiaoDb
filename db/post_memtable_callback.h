#pragma once

#include "xiaodb/status.h"
#include "xiaodb/types.h"

namespace XIAODB_NAMESPACE
{

    // Callback invoked after finishing writing to the memtable but berfor
    // publishing the sequence number to readers.
    // Note that with write-prepared/write-unprepared transactions with
    // two-write-queues, PreReleaseCallback is called publishing the
    // sequence numbers to readers.
    class PostMemTableCallback
    {
    public:
        virtual ~PostMemTableCallback() {}

        virtual Status operator()(SequenceNumber seq, bool disable_memtable) = 0;
    };
}