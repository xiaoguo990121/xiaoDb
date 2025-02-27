#pragma once

#include "db/dbformat.h"
#include "xiaodb/types.h"

namespace XIAODB_NAMESPACE
{

    class ReadCallback
    {
    public:
        explicit ReadCallback(SequenceNumber last_visible_seq)
            : max_visible_seq_(last_visible_seq) {}
        ReadCallback(SequenceNumber last_visible_seq, SequenceNumber min_uncommitted)
            : max_visible_seq_(last_visible_seq), min_uncommitted_(min_uncommitted) {}

        virtual ~ReadCallback() {}

        // Will be called to see if the seq number visible; if not moves on to
        // the next seq number.
        virtual bool IsVisibleFullCheck(SequenceNumber seq) = 0;

        inline bool IsVisible(SequenceNumber seq)
        {
            assert(min_uncommitted_ > 0);
            assert(min_uncommitted_ >= kMinUnCommittedSeq);
            if (seq < min_uncommitted_)
            {
                assert(seq <= max_visible_seq_);
                return true;
            }
            else if (max_visible_seq_ < seq)
            {
                assert(seq != 0);
                return false;
            }
            else
            {
                assert(seq != 0);
                return IsVisibleFullCheck(seq);
            }
        }

        inline SequenceNumber max_visiable_seq() { return max_visible_seq_; }

        virtual void Refresh(SequenceNumber seq) { max_visible_seq_ = seq; }

    protected:
        // The max visiable seq, it is usually the snapshot but could be larger if
        // transaction has its own writes written to db.
        SequenceNumber max_visible_seq_ = kMaxSequenceNumber;
        // Any seq less than min_uncommitted_ is committed.
        const SequenceNumber min_uncommitted_ = kMinUnCommittedSeq;
    };
}