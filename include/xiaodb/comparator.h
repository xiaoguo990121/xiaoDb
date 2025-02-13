#pragma once

#include <string>

#include "xiaodb/customizable.h"
#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{

    class Slice;

    class CompareInterface
    {
    public:
        virtual ~CompareInterface() {}

        virtual int Compare(const Slice &a, const Slice &b) const = 0;
    };

    class Comparator : public Customizable, public CompareInterface
    {
    public:
        Comparator() : timestamp_size_(0) {}

        Comparator(size_t ts_sz) : timestamp_size_(ts_sz) {}

        Comparator(const Comparator &orig) : timestamp_size_(orig.timestamp_size_) {}

        Comparator &operator=(const Comparator &rhs)
        {
            if (this != &rhs)
            {
                timestamp_size_ = rhs.timestamp_size_;
            }
            return *this;
        }

        ~Comparator() override {}

        static Status CreateFromString(const ConfigOptions &opts,
                                       const std::string &id,
                                       const Comparator **comp);
        static const char *Type() { return "Comparator"; }

        const char *Name() const override = 0;

        virtual bool Equal(const Slice &a, const Slice &b) const
        {
            return Compare(a, b) == 0;
        }

        virtual void FindShortestSeparator(std::string *start,
                                           const Slice &limit) const = 0;

        virtual void FindShortSuccessor(std::string *key) const = 0;

        virtual bool IsSameLengthImmediateSuccessor(const Slice &,
                                                    const Slice &) const
        {
            return false;
        }

        virtual bool CanKeysWithDifferentByteContentsBeEqual() const { return true; }

        virtual const Comparator *GetRootComparator() const { return this; }

        inline size_t timestamp_size() const { return timestamp_size_; }

        virtual Slice GetMaxTimestamp() const
        {
            if (timestamp_size_ == 0)
            {
                return Slice();
            }
            assert(false);
            return Slice();
        }

        virtual Slice GetMinTimestamp() const
        {
            if (timestamp_size_ == 0)
            {
                return Slice();
            }
            assert(false);
            return Slice();
        }

        virtual std::string TimestampToString(const Slice &) const
        {
            return "";
        }

        int CompareWithoutTimestamp(const Slice &a, const Slice &b) const
        {
            return CompareWithoutTimestamp(a, /*a_has_ts=*/true, b, /*b_has_ts=*/true);
        }

        virtual int CompareTimestamp(const Slice &,
                                     const Slice &) const
        {
            return 0;
        }

        virtual int CompareWithoutTimestamp(const Slice &a, bool /*a_has_ts*/,
                                            const Slice &b, bool /*b_has_ts*/) const
        {
            return Compare(a, b);
        }

        virtual bool EqualWithoutTimestamp(const Slice &a, const Slice &b) const
        {
            return 0 ==
                   CompareWithoutTimestamp(a, /*a_has_ts=*/true, b, /*b_has_ts=*/true);
        }

    private:
        size_t timestamp_size_;
    };

    // Return a builtin comparator that uses lexicographic ordering
    // on unsigned bytes, so the empty string is ordered before everything
    // else and a sufficiently long string of \xFF orders after anything.
    // CanKeysWithDifferentByteContentsBeEqual() == false
    // Returns an immortal pointer that must not be deleted by the caller.
    const Comparator *BytewiseComparator();

    // Return a builtin comparator that is the reverse ordering of
    // BytewiseComparator(), so the empty string is ordered after everything
    // else and a sufficiently long string of \xFF orders before anything.
    // CanKeysWithDifferentByteContentsBeEqual() == false
    // Returns an immortal pointer that must not be deleted by the caller.
    const Comparator *ReverseBytewiseComparator();

    // Returns a builtin comparator that enables user-defined timestamps (formatted
    // as uint64_t) while ordering the user key part without UDT with a
    // BytewiseComparator.
    // For the same user key with different timestamps, larger (newer) timestamp
    // comes first.
    const Comparator *BytewiseComparatorWithU64Ts();

    // Returns a builtin comparator that enables user-defined timestamps (formatted
    // as uint64_t) while ordering the user key part without UDT with a
    // ReverseBytewiseComparator.
    // For the same user key with different timestamps, larger (newer) timestamp
    // comes first.
    const Comparator *ReverseBytewiseComparatorWithU64Ts();

    // Decode a `U64Ts` timestamp returned by RocksDB to uint64_t.
    // When a column family enables user-defined timestamp feature
    // with `BytewiseComparatorWithU64Ts` or `ReverseBytewiseComparatorWithU64Ts`
    // comparator, the `Iterator::timestamp()` API returns timestamp in `Slice`
    // format. This util function helps to translate that `Slice` into an uint64_t
    // type.
    Status DecodeU64Ts(const Slice &ts, uint64_t *int_ts);

    // Encode an uint64_t timestamp into a U64Ts `Slice`, to be used as
    // `ReadOptions.timestamp` for a column family that enables user-defined
    // timestamp feature with `BytewiseComparatorWithU64Ts` or
    // `ReverseBytewiseComparatorWithU64Ts` comparator.
    // Be mindful that the returned `Slice` is backed by `ts_buf`. When `ts_buf`
    // is deconstructed, the returned `Slice` can no longer be used.
    Slice EncodeU64Ts(uint64_t ts, std::string *ts_buf);

    // Returns a `Slice` representing the maximum U64Ts timestamp.
    // The returned `Slice` is backed by some static storage, so it's valid until
    // program destruction.
    Slice MaxU64Ts();

    // Returns a `Slice` representing the minimum U64Ts timestamp.
    // The returned `Slice` is backed by some static storage, so it's valid until
    // program destruction.
    Slice MinU64Ts();
}