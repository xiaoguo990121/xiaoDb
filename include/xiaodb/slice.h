#pragma once

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

#include "xiaodb/cleanable.h"

namespace XIAODB_NAMESPACE
{

    class Slice
    {
    public:
        Slice() : data_(""), size_(0) {}

        Slice(const char *d, size_t n) : data_(d), size_(n) {}

        Slice(const std::string &s) : data_(s.data()), size_(s.size()) {}

        Slice(const std::string_view &sv) : data_(sv.data()), size_(sv.size()) {}

        Slice(const char *s) : data_(s) { size_ = (s == nullptr) ? 0 : strlen(s); }

        Slice(const struct SliceParts &parts, std::string *buf);

        const char *data() const { return data_; }

        size_t size() const { return size_; }

        bool empty() const { return size_ == 0; }

        char operator[](size_t n) const
        {
            assert(n < size());
            return data_[n];
        }

        void clear()
        {
            data_ = "";
            size_ = 0;
        }

        void remove_prefix(size_t n)
        {
            assert(n <= size());
            data_ += n;
            size_ -= n;
        }

        void remove_suffix(size_t n)
        {
            assert(n <= size());
            size_ -= n;
        }

        std::string ToString(bool hex = false) const;

        std::string_view ToStringView() const
        {
            return std::string_view(data_, size_);
        }

        bool DecodeHex(std::string *result) const;

        int compare(const Slice &b) const;

        bool starts_with(const Slice &x) const
        {
            return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
        }

        bool ends_with(const Slice &x) const
        {
            return ((size_ >= x.size_) &&
                    (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
        }

        size_t difference_offset(const Slice &b) const;

        const char *data_;
        size_t size_;
    };

    class PinnableSlice : public Slice, public Cleanable
    {
    public:
        PinnableSlice() { buf_ = &self_space_; }
        explicit PinnableSlice(std::string *buf) { buf_ = buf; }

        PinnableSlice(PinnableSlice &&other);
        PinnableSlice &operator=(PinnableSlice &&other);

        PinnableSlice(PinnableSlice &) = delete;
        PinnableSlice &operator=(PinnableSlice &) = delete;

        inline void PinSlice(const Slice &s, CleanupFunction f, void *arg1, void *arg2)
        {
            assert(!pinned_);
            pinned_ = true;
            data_ = s.data();
            size_ = s.size();
            RegisterCleanup(f, arg1, arg2);
            assert(pinned_);
        }

        inline void PinSlice(const Slice &s, Cleanable *cleanable)
        {
            assert(!pinned_);
            pinned_ = true;
            data_ = s.data();
            size_ = s.size();
            if (cleanable != nullptr)
            {
                cleanable->DelegateCleanupsTo(this);
            }
            assert(pinned_);
        }

        inline void PinSelf(const Slice &slice)
        {
            assert(!pinned_);
            buf_->assign(slice.data(), slice.size());
            data_ = buf_->data();
            size_ = buf_->size();
            assert(!pinned_);
        }

        inline void PinSelf()
        {
            assert(!pinned_);
            data_ = buf_->data();
            size_ = buf_->size();
            assert(!pinned_);
        }

        void remove_suffix(size_t n)
        {
            assert(n <= size());
            if (pinned_)
            {
                size_ -= n;
            }
            else
            {
                buf_->erase(size() - n, n);
                PinSelf();
            }
        }

        void remove_prefix(size_t n)
        {
            assert(n <= size());
            if (pinned_)
            {
                data_ += n;
                size_ -= n;
            }
            else
            {
                buf_->erase(0, n);
                PinSelf();
            }
        }

        void Reset()
        {
            Cleanable::Reset();
            pinned_ = false;
            size_ = 0;
        }

        inline std::string *GetSelf() { return buf_; }

        inline bool IsPinned() const { return pinned_; }

    private:
        friend class PinnableSlice4Test;
        std::string self_space_;
        std::string *buf_;
        bool pinned_ = false;
    };

    struct SliceParts
    {
        SliceParts(const Slice *_parts, int _num_parts)
            : parts(_parts), num_parts(_num_parts) {}
        SliceParts() : parts(nullptr), num_parts(0) {}

        const Slice *parts;
        int num_parts;
    };

    inline bool operator==(const Slice &x, const Slice &y)
    {
        return ((x.size() == y.size()) &&
                (memcmp(x.data(), y.data(), x.size()) == 0));
    }

    inline bool operator!=(const Slice &x, const Slice &y) { return !(x == y); }

    inline int Slice::compare(const Slice &b) const
    {
        assert(data_ != nullptr && b.data_ != nullptr);
        const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
        int r = memcmp(data_, b.data_, min_len);
        if (r == 0)
        {
            if (size_ < b.size_)
                r = -1;
            else if (size_ > b.size_)
                r = +1;
        }
        return r;
    }

    inline size_t Slice::difference_offset(const Slice &b) const
    {
        size_t off = 0;
        const size_t len = (size_ < b.size_) ? size_ : b.size_;
        for (; off < len; off++)
        {
            if (data_[off] != b.data_[off])
                break;
        }
        return off;
    }

}