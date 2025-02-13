#pragma once
#include <assert.h>
#include <stdlib.h>

#include <atomic>

#include "memory/allocator.h"
#include "port/port.h"
#include "util/random.h"

namespace XIAODB_NAMESPACE
{

    template <typename Key, class Comparator>
    class SkipList
    {
    private:
        struct Node;

    public:
        /**
         * @brief Create a new SkipList object that will use "cmp" for comparing keys,
         * and will allocate memory using "*allocator". Objects allocated in the
         * allocator must remain allocated for the lifetime of the skiplist object.
         *
         * @param cmp
         * @param allocator
         * @param max_height
         * @param branching_factor
         */
        explicit SkipList(Comparator cmp, Allocator *allocator,
                          int32_t max_height = 12, int32_t branching_factor = 4);

        SkipList(const SkipList &) = delete;
        void operator=(const SkipList &) = delete;

        void Insert(const Key &key);

        bool Contains(const Key &key) const;

        uint64_t ApproximateNumEntries(const Slice &start_ikey,
                                       const Slice &end_ikey) const;

        class Iterator
        {
        public:
            explicit Iterator(const SkipList *list);

            void SetList(const SkipList *list);

            bool Valid() const;

            const Key &key() const;

            void Next();

            void Prev();

            void Seek(const Key &target);

            void SeekForPrev(const Key &target);

            void SeekToFirst();

            void SeekToLast();

        private:
            const SkipList *list_;
            Node *node_;
        };

    private:
        const uint16_t kMaxHeight_;
        const uint16_t kBranching_;
        const uint32_t kScaledInverseBranching_;

        Comparator const compare_;
        Allocator *const allocator_;

        Node *const head_;

        std::atomic<int> max_height_;

        Node **prev_;
        int32_t prev_height_;

        inline int GewMaxHeight() const
        {
            return max_height_.load(std::memory_order_relaxed);
        }

        Node *NewNode(const Key &key, int height);
        int RandomHeight();
        bool Equal(const Key &a, const Key &b) const { return (compare_(a, b) == 0); }
        bool LessThan(const Key &a, const Key &b) const
        {
            return (compare_(a, b) < 0);
        }

        // Return true if key is greater than the data stored in "n"
        bool KeyIsAfterNode(const Key &key, Node *n) const;

        // Returns the earliest node with a key >= key.
        // Return nullptr if there is no such node.
        Node *FindGreaterOrEqual(const Key &key) const;

        // Return the latest node with a key < key.
        // Return head_ if there is no such node.
        // Fills prev[level] with pointer to previous node at "level" for every
        // level in [0..max_height_-1], if prev is non-null.
        Node *FindLessThan(const Key &key, Node **prev = nullptr) const;

        // Return the last node in the list.
        // Return head_ if list is empty.
        Node *FindLast() const;
    };

    template <typename Key, class Comparator>
    struct SkipList<Key, Comparator>::Node
    {
        explicit Node(const Key &k) : key(k) {}

        Key const key;

        Node *Next(int n)
        {
            assert(n >= 0);
            return (next_[n].load(std::memory_order_acquire));
        }
        void SetNext(int n, Node *x)
        {
            assert(n >= 0);
            next_[n].store(x, std::memory_order_release);
        }

        Node *NoBarrier_Next(int n)
        {
            assert(n >= 0);
            return next_[n].load(std::memory_order_relaxed);
        }
        void NoBarrier_SetNext(int n, Node *x)
        {
            assert(n >= 0);
            next_[n].store(x, std::memory_order_relaxed);
        }

    private:
        std::atomic<Node *> next_[1];
    };

    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::NewNode(
        const Key &key, int height)
    {
        char *mem = allocator_->AllocateAligned(
            sizeof(Node) + sizeof(std::atomic<Node *>) * (height - 1));
        return new (mem) Node(key);
    }

    template <typename Key, class Comparator>
    inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList *list)
    {
        SetList(list);
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SetList(const SkipList *list)
    {
        list_ = list;
        node_ = nullptr;
    }

    template <typename Key, class Comparator>
    inline bool SkipList<Key, Comparator>::Iterator::Valid() const
    {
        return node_ != nullptr;
    }

    template <typename Key, class Comparator>
    inline const Key &SkipList<Key, Comparator>::Iterator::key() const
    {
        assert(Valid());
        return node_->key;
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Next()
    {
        assert(Valid());
        node_ = node_->Next(0);
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Prev()
    {
        // Instead of using explicit "prev" links, we just search for the
        // last node that falls before key.
        assert(Valid());
        node_ = list_->FindLessThan(node_->key);
        if (node_ == list_->head_)
        {
            node_ = nullptr;
        }
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Seek(const Key &target)
    {
        node_ = list_->FindGreaterOrEqual(target);
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SeekForPrev(
        const Key &target)
    {
        Seek(target);
        if (!Valid())
        {
            SeekToLast();
        }
        while (Valid() && list_->LessThan(target, key()))
        {
            Prev();
        }
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SeekToFirst()
    {
        node_ = list_->head_->Next(0);
    }

    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SeekToLast()
    {
        node_ = list_->FindLast();
        if (node_ == list_->head_)
        {
            node_ = nullptr;
        }
    }

    template <typename Key, class Comparator>
    int SkipList<Key, Comparator>::RandomHeight()
    {
        auto rnd = Random::GetTLSInstance();

        // Increase height with probability 1 in kBranching
        int height = 1;
        while (height < kMaxHeight_ && rnd->Next() < kScaledInverseBranching_)
        {
            height++;
        }
        assert(height > 0);
        assert(height <= kMaxHeight_);
        return height;
    }

    template <typename Key, class Comparator>
    bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key &key, Node *n) const
    {
        // nullptr n is considered infinite
        return (n != nullptr) && (compare_(n->key, key) < 0);
    }

    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node *
    SkipList<Key, Comparator>::FindGreaterOrEqual(const Key &key) const
    {
        // Note: It looks like we could reduce duplication by implementing
        // this function as FindLessThan(key)->Next(0), but we wouldn't be able
        // to exit early on equality and the result wouldn't even be correct.
        // A concurrent insert might occur after FindLessThan(key) but before
        // we get a chance to call Next(0).
        Node *x = head_;
        int level = GetMaxHeight() - 1;
        Node *last_bigger = nullptr;
        while (true)
        {
            assert(x != nullptr);
            Node *next = x->Next(level);
            // Make sure the lists are sorted
            assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
            // Make sure we haven't overshot during our search
            assert(x == head_ || KeyIsAfterNode(key, x));
            int cmp =
                (next == nullptr || next == last_bigger) ? 1 : compare_(next->key, key);
            if (cmp == 0 || (cmp > 0 && level == 0))
            {
                return next;
            }
            else if (cmp < 0)
            {
                // Keep searching in this list
                x = next;
            }
            else
            {
                // Switch to next list, reuse compare_() result
                last_bigger = next;
                level--;
            }
        }
    }

    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node *
    SkipList<Key, Comparator>::FindLessThan(const Key &key, Node **prev) const
    {
        Node *x = head_;
        int level = GetMaxHeight() - 1;
        // KeyIsAfter(key, last_not_after) is definitely false
        Node *last_not_after = nullptr;
        while (true)
        {
            assert(x != nullptr);
            Node *next = x->Next(level);
            assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
            assert(x == head_ || KeyIsAfterNode(key, x));
            if (next != last_not_after && KeyIsAfterNode(key, next))
            {
                // Keep searching in this list
                x = next;
            }
            else
            {
                if (prev != nullptr)
                {
                    prev[level] = x;
                }
                if (level == 0)
                {
                    return x;
                }
                else
                {
                    // Switch to next list, reuse KeyIUsAfterNode() result
                    last_not_after = next;
                    level--;
                }
            }
        }
    }

    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::FindLast()
        const
    {
        Node *x = head_;
        int level = GetMaxHeight() - 1;
        while (true)
        {
            Node *next = x->Next(level);
            if (next == nullptr)
            {
                if (level == 0)
                {
                    return x;
                }
                else
                {
                    // Switch to next list
                    level--;
                }
            }
            else
            {
                x = next;
            }
        }
    }

    template <typename Key, class Comparator>
    uint64_t SkipList<Key, Comparator>::ApproximateNumEntries(
        const Slice &start_ikey, const Slice &end_ikey) const
    {
        // See InlineSkipList<Comparator>::ApproximateNumEntries() (copy-paste)
        Node *lb = head_;
        Node *ub = nullptr;
        uint64_t count = 0;
        for (int level = GetMaxHeight() - 1; level >= 0; level--)
        {
            auto sufficient_samples = static_cast<uint64_t>(level) * kBranching_ + 10U;
            if (count >= sufficient_samples)
            {
                // No more counting; apply powers of kBranching and avoid floating point
                count *= kBranching_;
                continue;
            }
            count = 0;
            Node *next;
            // Get a more precise lower bound (for start key)
            for (;;)
            {
                next = lb->Next(level);
                if (next == ub)
                {
                    break;
                }
                assert(next != nullptr);
                if (compare_(next->Key(), start_ikey) >= 0)
                {
                    break;
                }
                lb = next;
            }
            // Count entries on this level until upper bound (for end key)
            for (;;)
            {
                if (next == ub)
                {
                    break;
                }
                assert(next != nullptr);
                if (compare_(next->Key(), end_ikey) >= 0)
                {
                    // Save refined upper bound to potentially save key comparison
                    ub = next;
                    break;
                }
                count++;
                next = next->Next(level);
            }
        }
        return count;
    }

    template <typename Key, class Comparator>
    SkipList<Key, Comparator>::SkipList(const Comparator cmp, Allocator *allocator,
                                        int32_t max_height,
                                        int32_t branching_factor)
        : kMaxHeight_(static_cast<uint16_t>(max_height)),
          kBranching_(static_cast<uint16_t>(branching_factor)),
          kScaledInverseBranching_((Random::kMaxNext + 1) / kBranching_),
          compare_(cmp),
          allocator_(allocator),
          head_(NewNode(0 /* any key will do */, max_height)),
          max_height_(1),
          prev_height_(1)
    {
        assert(max_height > 0 && kMaxHeight_ == static_cast<uint32_t>(max_height));
        assert(branching_factor > 0 &&
               kBranching_ == static_cast<uint32_t>(branching_factor));
        assert(kScaledInverseBranching_ > 0);
        // Allocate the prev_ Node* array, directly from the passed-in allocator.
        // prev_ does not need to be freed, as its life cycle is tied up with
        // the allocator as a whole.
        prev_ = reinterpret_cast<Node **>(
            allocator_->AllocateAligned(sizeof(Node *) * kMaxHeight_));
        for (int i = 0; i < kMaxHeight_; i++)
        {
            head_->SetNext(i, nullptr);
            prev_[i] = head_;
        }
    }

    template <typename Key, class Comparator>
    void SkipList<Key, Comparator>::Insert(const Key &key)
    {
        // fast path for sequential insertion
        if (!KeyIsAfterNode(key, prev_[0]->NoBarrier_Next(0)) &&
            (prev_[0] == head_ || KeyIsAfterNode(key, prev_[0])))
        {
            assert(prev_[0] != head_ || (prev_height_ == 1 && GetMaxHeight() == 1));

            // Outside of this method prev_[1..max_height_] is the predecessor
            // of prev_[0], and prev_height_ refers to prev_[0].  Inside Insert
            // prev_[0..max_height - 1] is the predecessor of key.  Switch from
            // the external state to the internal
            for (int i = 1; i < prev_height_; i++)
            {
                prev_[i] = prev_[0];
            }
        }
        else
        {
            // TODO(opt): we could use a NoBarrier predecessor search as an
            // optimization for architectures where memory_order_acquire needs
            // a synchronization instruction.  Doesn't matter on x86
            FindLessThan(key, prev_);
        }

        // Our data structure does not allow duplicate insertion
        assert(prev_[0]->Next(0) == nullptr || !Equal(key, prev_[0]->Next(0)->key));

        int height = RandomHeight();
        if (height > GetMaxHeight())
        {
            for (int i = GetMaxHeight(); i < height; i++)
            {
                prev_[i] = head_;
            }
            // fprintf(stderr, "Change height from %d to %d\n", max_height_, height);

            // It is ok to mutate max_height_ without any synchronization
            // with concurrent readers.  A concurrent reader that observes
            // the new value of max_height_ will see either the old value of
            // new level pointers from head_ (nullptr), or a new value set in
            // the loop below.  In the former case the reader will
            // immediately drop to the next level since nullptr sorts after all
            // keys.  In the latter case the reader will use the new node.
            max_height_.store(height, std::memory_order_relaxed);
        }

        Node *x = NewNode(key, height);
        for (int i = 0; i < height; i++)
        {
            // NoBarrier_SetNext() suffices since we will add a barrier when
            // we publish a pointer to "x" in prev[i].
            x->NoBarrier_SetNext(i, prev_[i]->NoBarrier_Next(i));
            prev_[i]->SetNext(i, x);
        }
        prev_[0] = x;
        prev_height_ = height;
    }

    template <typename Key, class Comparator>
    bool SkipList<Key, Comparator>::Contains(const Key &key) const
    {
        Node *x = FindGreaterOrEqual(key);
        if (x != nullptr && Equal(key, x->key))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}