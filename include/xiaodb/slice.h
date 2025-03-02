#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include "xiaodb/cleanable.h"

namespace XIAODB_NAMESPACE
{

    /**
     * @brief Slice is a simple structure containing a pointer into some external
     * storage and a size. The user of a Slice must ensure that the slice is not
     * used after the corresponding external storage has been deallocated.
     *
     * Multiple threads can invoke const methods on a Slice without external
     * synchroization, but if any of the threads may call a no-const method,
     * all threads accessing the same Slice must use external synchronization.
     */
    class Slice
    {
    public:
        // 创建一个空Slice
        Slice() : data_(""), size_(0) {}

        // 创建一个引用d[0, n-1]的Slice
        Slice(const char *d, size_t n) : data_(d), size_(n) {}

        // 创建一个引用字符串s的Slice
        Slice(const std::string &s) : data_(s.data()), size_(s.size()) {}

        // 创建一个引用string_view的Slice
        Slice(const std::string_view &sv) : data_(sv.data()), size_(sv.size()) {}

        // 创建一个s[0, strlen(s) - 1]的Slice
        Slice(const char *s) : data_(s) { size_ = (s == nullptr) ? 0 : strlen(s); }

        // 从SliceParts 创建一个Slice对象，使用buf作为存储，buf必须在返回的slice存在期间一直存在
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

        // 返回一个包含引用数据副本的字符串
        // 当hex为true时，返回一个长度为两倍的十六进制编码字符串(0-9A-F)
        std::string ToString(bool hex = false) const;

        std::string_view ToStringView() const
        {
            return std::string_view(data_, size_);
        }

        // 将当前 Slice 解释为十六进制字符串并解码到 result 中，
        // 如果成功则返回 true，如果这不是一个有效的十六进制字符串
        // （例如不是来自 Slice::ToString(true)）则返回 false。
        // 此 Slice 应包含偶数个 0-9A-F 字符，也接受小写（a-f）
        bool DecodeHex(std::string *result) const;

        // 三向比较。返回值：
        //   <  0 当且仅当 "*this" <  "b"，
        //   == 0 当且仅当 "*this" == "b"，
        //   >  0 当且仅当 "*this" >  "b"
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

        // 比较两个Slice并返回它们不同的第一个字节的位置
        size_t difference_offset(const Slice &b) const;

        const char *data_;
        size_t size_;
    };

    /**
     * @brief PinnableSlice用于引用内存中的数据，从而避免了memcpy带来的开销，它通过将数据
     * 固定在内存中，确保用户处理数据期间，数据不会被意外删除或修改
     *
     */
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
        std::string self_space_; // 一个std::string类型的私有成员变量，用于存储数据
        std::string *buf_;       // 一个指向std::string的指针，指向数据存储的位置，默认指向self_space_
        bool pinned_ = false;    // 用于标记数据是否已经被固定
    };

    // A set of Slices that are virtually concatenated together. 'parts' points
    // to an array of Slices. The number of elements in the array is 'num_parts'
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