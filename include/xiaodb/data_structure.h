#pragma once

#include <assert.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    namespace detail
    {
        int CountTrailingZeroBitsForSmallEnumSet(uint64_t);
    }

    // Represents a set of values of some enum type with a small number of
    // possible enumerators. For now, it supports enums where no enumerator
    // exceeds 63 when converted to int.
    template <typename ENUM_TYPE, ENUM_TYPE MAX_ENUMERATOR>
    class SmallEnumSet
    {
    private:
        using StateT = uint64_t;
        static constexpr int kStateBits = sizeof(StateT) * 8;
        static constexpr int kMaxMax = kStateBits - 1;
        static constexpr int kMaxValue = static_cast<int>(MAX_ENUMERATOR);
        static_assert(kMaxValue >= 0);
        static_assert(kMaxValue <= kMaxMax);

    public:
        SmallEnumSet() : state_(0) {}

        template <class... TRest>
        constexpr SmallEnumSet(const ENUM_TYPE e, TRest... rest)
        {
            *this = SmallEnumSet(rest...).With(e);
        }

        static constexpr SmallEnumSet All()
        {
            StateT tmp = StateT{1} << kMaxValue;
            return SmallEnumSet(RawStateMarker(), tmp | (tmp - 1));
        }

        bool operator==(const SmallEnumSet &that) const
        {
            return this->state_ == that.state_;
        }
        bool operator!=(const SmallEnumSet &that) const
        {
            return !(*this == that);
        }

        bool Contains(const ENUM_TYPE e) const
        {
            int value = static_cast<int>(e);
            assert(value >= 0 && value <= kMaxValue);
            StateT tmp = 1;
            return state_ & (tmp << value);
        }

        bool empty() const { return state_ == 0; }

        class const_iterator
        {
        public:
            const_iterator(const const_iterator &that) = default;
            const_iterator &operator=(const const_iterator &that) = default;

            const_iterator(const_iterator &&that) noexcept = default;
            const_iterator &operator=(const_iterator &&that) noexcept = default;

            bool operator==(const const_iterator &that) const
            {
                assert(set_ == that.set_);
                return this->pos_ == that.pos_;
            }

            bool operator!=(const const_iterator &that) const
            {
                return !(*this == that);
            }

            const_iterator &operator++()
            {
                if (pos_ < kMaxValue)
                {
                    pos_ = set_->SkipUnset(pos_ + 1);
                }
                else
                {
                    pos_ = kStateBits;
                }
                return *this;
            }

            const_iterator operator++(int)
            {
                auto old = *this;
                ++*this;
                return old;
            }

            ENUM_TYPE operator*() const
            {
                assert(pos_ <= kMaxValue);
                return static_cast<ENUM_TYPE>(pos_);
            }

        private:
            friend class SmallEnumSet;
            const_iterator(const SmallEnumSet *set, int pos) : set_(set), pos_(pos) {}
            const SmallEnumSet *set_;
            int pos_;
        };

        const_iterator begin() const { return const_iterator(this, SkipUnset(0)); }

        const_iterator end() const { return const_iterator(this, kStateBits); }

        bool Add(const ENUM_TYPE e)
        {
            int value = static_cast<int>(e);
            assert(value >= 0 && value <= kMaxValue);
            StateT old_state = state_;
            state_ |= (StateT{1} << value);
            return old_state != state_;
        }

        bool Remove(const ENUM_TYPE e)
        {
            int value = static_cast<int>(e);
            assert(value >= 0 && value <= kMaxValue);
            StateT old_state = state_;
            state_ &= ~(StateT{1} << value);
            return old_state != state_;
        }

        constexpr SmallEnumSet With(const ENUM_TYPE e) const
        {
            int value = static_cast<int>(e);
            assert(value >= 0 && value <= kMaxValue);
            return SmallEnumSet(RawStateMarker(), state_ | (StateT{1} << value));
        }

        template <class... TRest>
        constexpr SmallEnumSet With(const ENUM_TYPE e1, const ENUM_TYPE e2,
                                    TRest... rest) const
        {
            return With(e1).With(e2, rest...);
        }

        constexpr SmallEnumSet Without(const ENUM_TYPE e) const
        {
            int value = static_cast<int>(e);
            assert(value >= 0 && value <= kMaxValue);
            return SmallEnumSet(RawStateMarker(), state_ & ~(StateT{1} << value));
        }
        template <class... TRest>
        constexpr SmallEnumSet Without(const ENUM_TYPE e1, const ENUM_TYPE e2,
                                       TRest... rest) const
        {
            return Without(e1).Without(e2, rest...);
        }

    private:
        int SkipUnset(int pos) const
        {
            StateT tmp = state_ >> pos;
            if (tmp == 0)
            {
                return kStateBits;
            }
            else
            {
                return pos + detail::CountTrailingZeroBitsForSmallEnumSet(tmp);
            }
        }

        struct RawStateMarker
        {
        };
        explicit SmallEnumSet(RawStateMarker, StateT state) : state_(state) {}

        StateT state_;
    };
}