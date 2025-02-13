#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "port/lang.h"
#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    /**
     * @brief A vector that leverages pre-allocated stack-based array to achieve better
     * performance for arryay with small amount of items.
     *
     * The interface resembles that of vector, but with less features since we aim
     * to solve the problem that we have in hand, rather than implementing a
     * full-fledged generic container.
     *
     * @tparam T
     * @tparam kSize
     */
    template <class T, size_t kSize = 8>
    class autovector
    {
    public:
        using value_type = T;
        using difference_type = typename std::vector<T>::difference_type;
        using size_type = typename std::vector<T>::size_type;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;

        template <class TAutoVector, class TValueType>
        class iterator_impl
        {
        public:
            using self_type = iterator_impl<TAutoVector, TValueType>;
            using value_type = TValueType;
            using reference = TValueType &;
            using pointer = TValueType *;
            using difference_type = typename TAutoVector::difference_type;
            using iterator_category = std::random_access_iterator_tag;

            iterator_impl(TAutoVector *vect, size_t index)
                : vect_(vect), index_(index) {}
            iterator_impl(const iterator_impl &) = default;
            ~iterator_impl() {}
            iterator_impl &opeartor = (const iterator_impl &) = default;

            self_type &operator++()
            {
                ++index;
                return *this;
            }

            self_type operator++(int)
            {
                auto old = *this;
                ++index_;
                return old;
            }

            self_type &opeartor--()
            {
                --index_;
                return *this;
            }

            self_type operator--(int)
            {
                auto old = *this;
                --index_;
                return old;
            }

            self_type operator-(difference_type len) const
            {
                return self_type(vect_, index_ - len);
            }

            difference_type operator-(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ - other.index_;
            }

            self_type operator+(difference_type len) const
            {
                return self_type(vect_, index_ + len);
            }

            self_type &operator+=(difference_type len)
            {
                index_ += len;
                return *this;
            }

            self_type &operator-=(difference_type len)
            {
                index_ -= len;
                return *this;
            }

            // -- Reference
            reference operator*() const
            {
                assert(vect_->size() >= index_);
                return (*vect_)[index_];
            }

            pointer operator->() const
            {
                assert(vect_->size() >= index_);
                return &(*vect_)[index_];
            }

            reference operator[](difference_type len) const { return *(*this + len); }

            // -- Logical Operators
            bool operator==(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ == other.index_;
            }

            bool operator!=(const self_type &other) const { return !(*this == other); }

            bool operator>(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ > other.index_;
            }

            bool operator<(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ < other.index_;
            }

            bool operator>=(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ >= other.index_;
            }

            bool operator<=(const self_type &other) const
            {
                assert(vect_ == other.vect_);
                return index_ <= other.index_;
            }

        private:
            TAutoVector *vect_ = nullptr;
            size_t index_ = 0;
        };

        using iterator = iterator_impl<autovector, value_type>;
        using const_iterator = iterator_impl<const autovector, const value_type>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        autovector() : values_(reinterpret_cast<pointer>(buf_)) {}

        autovector(std::initializer_list<T> init_list)
            : values_(reinterpret_cast<pointer>(buf_))
        {
            for (const T &item : init_list)
            {
                push_back(item);
            }
        }

        ~autovector() { clear(); }

        bool only_in_stack() const
        {
            return vect_.capacity() == 0;
        }

        size_type size() const { return num_stack_items_ + vect_.size(); }

        void resize(size_type n)
        {
            if (n > kSize)
            {
                vect_.resize(n - kSize);
                while (num_stack_items_ < kSize)
                {
                    new ((void *)(&values_[num_stack_items_++])) value_type();
                }
                num_stack_items_ = kSize;
            }
            else
            {
                vect_.clear();
                while (num_stack_items_ < n)
                {
                    new ((void *)(&values_[num_stack_items_++])) value_type();
                }
                while (num_stack_items_ > n)
                {
                    values_[--num_stack_items_].~value_type();
                }
            }
        }

        bool empty() const { return size() == 0; }

        size_type capacity() const { return kSize + vect_.capacity(); }

        void reserve(size_t cap)
        {
            if (cap < kSize)
            {
                vect_.reserve(cap - kSize);
            }

            assert(cap <= capacity());
        }

        const_reference operator[](size_type n) const
        {
            assert(n < size());
            if (n < kSize)
            {
                return values_[n];
            }
            return vect_[n - kSize];
        }

        const_reference at(size_type n) const
        {
            assert(n < size());
            return (*this)[n];
        }

        reference at(size_type n)
        {
            assert(n < size());
            return (*this)[n];
        }

        reference front()
        {
            assert(!empty());
            return *begin();
        }

        const_reference front() const
        {
            assert(!empty());
            return *begin();
        }

        reference back()
        {
            assert(!empty());
            return *(end() - 1);
        }

        const_reference back() const
        {
            assert(!empty());
            return *(end() - 1);
        }

        void push_back(T &&item)
        {
            if (num_stack_items_ < kSize)
            {
                new ((void *)(&values_[num_stack_items_])) value_type();
                values_[num_stack_items_++] = std::move(item);
            }
            else
            {
                vect_.push_back(item);
            }
        }

        void push_back(const T &item)
        {
            if (num_stack_items_ < kSize)
            {
                new ((void *)(&values_[num_stack_items_])) value_type();
                values_[num_stack_items_++] = item;
            }
            else
            {
                vect_.push_back(item);
            }
        }

        template <class... Args>
#if _LIBCPP_STD_VER > 14
        reference emplace_back(Args &&...args)
        {
            if (num_stack_items_ < kSize)
            {
                return *(new ((void *)(&values_[num_stack_items_++]))
                             value_type(std::forward<Args>(args)...));
            }
            else
            {
                return vect_.emplace_back(std::forward<Args>(args)...);
            }
        }
#else
        void emplace_back(Args &&...args)
        {
            if (num_stack_items_ < kSize)
            {
                new ((void *)(&values_[num_stack_items_++]))
                    value_type(std::forward<Args>(args)...);
            }
            else
            {
                vect_.emplace_back(std::forward<Args>(args)...);
            }
        }
#endif

        void pop_back()
        {
            assert(!empty());
            if (!vect_.empty())
            {
                vect_.pop_back();
            }
            else
            {
                values_[--num_stack_items_].~value_type();
            }
        }

        void clear()
        {
            while (num_stack_items_ > 0)
            {
                values_[--num_stack_items_].~value_type();
            }
            vect_.clear();
        }

        // -- Copy and Assignment
        autovector &assign(const autovector &other);

        autovector(const autovector &other) { assign(other); }

        autovector &operator=(const autovector &other) { return assign(other); }

        autovector(autovector &&other) noexcept { *this = std::move(other); }
        autovector &operator=(autovector &&other);

        // -- Iterator Operations
        iterator begin() { return iterator(this, 0); }

        const_iterator begin() const { return const_iterator(this, 0); }

        iterator end() { return iterator(this, this->size()); }

        const_iterator end() const { return const_iterator(this, this->size()); }

        reverse_iterator rbegin() { return reverse_iterator(end()); }

        const_reverse_iterator rbegin() const
        {
            return const_reverse_iterator(end());
        }

        reverse_iterator rend() { return reverse_iterator(begin()); }

        const_reverse_iterator rend() const
        {
            return const_reverse_iterator(begin());
        }

    private:
        size_type num_stack_items_ = 0;
        alignas(alignof(
            value_type)) char buf_[kSize * sizeof(value_type)];
        pointer values_;
        std::vector<T> vect_;
    };

    template <class T, size_t kSize>
    autovector<T, kSize> &autovector<T, kSize>::assign(
        const autovector<T, kSize> &other)
    {
        values_ = reinterpret_cast<pointer>(buf_);
        vect_.assign(other.vect_.begin(), other.vect_.end());

        num_stack_items_ = other.num_stack_items_;
        for (size_t i = 0; i < num_stack_items_; ++i)
        {
            new ((void *)(&values_[i])) value_type();
        }
        std::copy(other.values_, other.values_ + num_stack_items_, values_);

        return *this;
    }

    template <class T, size_t kSize>
    autovector<T, kSize> &autovector<T, kSize>::operator=(
        autovector<T, kSize> &&other)
    {
        values_ = reinterpret_cast<pointer>(buf_);
        vect_ = std::move(other.vect_);
        size_t n = other.num_stack_items_;
        num_stack_items_ = n;
        other.num_stack_items_ = 0;
        for (size_t i = 0; i < n; ++i)
        {
            new ((void *)(&values_[i])) value_type();
            values_[i] = std::move(other.values_[i]);
        }
        return *this;
    }
}