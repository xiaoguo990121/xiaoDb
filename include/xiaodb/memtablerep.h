#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <memory>
#include <stdexcept>
#include <unordered_set>

#include "xiaodb/customizable.h"
#include "xiaodb/slice.h"

namespace XIAODB_NAMESPACE
{
    class Arena;
    class Allocator;
    class LookupKey;
    class SliceTransform;
    class Logger;
    struct DBOptions;

    using KeyHandle = void *;

    Slice GetLengthPrefixedSlice(const char *data);

    class MemTableRep
    {
    public:
        /**
         * @brief KeyComparator provides a means to compare keys, which are internal keys
         * concatenated with values.
         *
         */
        class KeyComparator
        {
        public:
            using DecodeType = XIAODB_NAMESPACE::Slice;

            virtual DecodeType decode_key(const char *key) const
            {
                return GetLengthPrefixedSlice(key);
            }

            virtual int operator()(const char *prefix_len_key1,
                                   const char *prefix_len_key2) const = 0;

            virtual int operator()(const char *prefix_len_key,
                                   const Slice &key) const = 0;

            virtual ~KeyComparator() {}
        };

        explicit MemTableRep(Allocator *allocator) : allocator_(allocator) {}

        // Allocate a buf of len size for storing key. The idea is that a
        // specific memtable representation knows its underlying data structure
        // better. By allowing it to allocate memory, it can possibly put
        // correlated stuff in consecutive memory area to make processor
        // prefetching more efficient.
        virtual KeyHandle Allocate(const size_t len, char **buf);

        // Insert key into collection. (The caller will pack key and value into a
        // single buffer and pass that in as the parameter to Insert).
        // REQUIRES: nothing that compares equal to key is currently in the
        // collection, and no concurrent modifications to the table in progress
        virtual void Insert(KeyHandle handle) = 0;

        // Same as ::Insert
        // Returns false if MemTableRepFactory::CanHandleDuplicateKey() is true and
        // the <key, seq> already exists.
        virtual bool InsertKey(KeyHandle handle)
        {
            Insert(handle);
            return true;
        }

        // Same as Insert(), but in additional pass a hint to insert location for
        // the key. If hint points to nullptr, a new hint will be populated.
        // otherwise the hint will be updated to reflect the last insert location.
        //
        // Currently only skip-list based memtable implement the interface. Other
        // implementations will fallback to Insert() by default.
        virtual void InsertWithHint(KeyHandle handle, void ** /*hint*/)
        {
            // Ignore the hint by default.
            Insert(handle);
        }

        // Same as ::InsertWithHint
        // Returns false if MemTableRepFactory::CanHandleDuplicatedKey() is true and
        // the <key, seq> already exists.
        virtual bool InsertKeyWithHint(KeyHandle handle, void **hint)
        {
            InsertWithHint(handle, hint);
            return true;
        }

        // Same as ::InsertWithHint, but allow concurrent write
        //
        // If hint points to nullptr, a new hint will be allocated on heap, otherwise
        // the hint will be updated to reflect the last insert location. The hint is
        // owned by the caller and it is the caller's responsibility to delete the
        // hint later.
        //
        // Currently only skip-list based memtable implement the interface. Other
        // implementations will fallback to InsertConcurrently() by default.
        virtual void InsertWithHintConcurrently(KeyHandle handle, void ** /*hint*/)
        {
            // Ignore the hint by default.
            InsertConcurrently(handle);
        }

        // Same as ::InsertWithHintConcurrently
        // Returns false if MemTableRepFactory::CanHandleDuplicatedKey() is true and
        // the <key, seq> already exists.
        virtual bool InsertKeyWithHintConcurrently(KeyHandle handle, void **hint)
        {
            InsertWithHintConcurrently(handle, hint);
            return true;
        }

        // Like Insert(handle), but may be called concurrent with other calls
        // to InsertConcurrently for other handles.
        //
        // Returns false if MemTableRepFactory::CanHandleDuplicatedKey() is true and
        // the <key, seq> already exists.
        virtual void InsertConcurrently(KeyHandle handle);

        // Same as ::InsertConcurrently
        // Returns false if MemTableRepFactory::CanHandleDuplicatedKey() is true and
        // the <key, seq> already exists.
        virtual bool InsertKeyConcurrently(KeyHandle handle)
        {
            InsertConcurrently(handle);
            return true;
        }

        // Returns true if an entry that compares equal to key is in the collection.
        virtual bool Contains(const char *key) const = 0;

        // Notify this table rep that it will no longer be added to. By default,
        // does nothing. After MarkReadOnly() is called, this table rep will
        // not be written to (ie No more calls to Allocate(), Insert(),
        // or any writes done directly to entries accessed through the iterator.)
        virtual void MarkReadOnly() {}

        // Notify this table rep that it has been flushed to stable storage.
        // By default, does nothing.
        //
        // Invariant: MarkReadOnly() is called, before MarkFlushed().
        // Note that this method if overridden, should not run for an extended period
        // of time. Otherwise, RocksDB may be blocked.
        virtual void MarkFlushed() {}

        // Look up key from the mem table, since the first key in the mem table whose
        // user_key matches the one given k, call the function callback_func(), with
        // callback_args directly forwarded as the first parameter, and the mem table
        // key as the second parameter. If the return value is false, then terminates.
        // Otherwise, go through the next key.
        //
        // It's safe for Get() to terminate after having finished all the potential
        // key for the k.user_key(), or not.
        //
        // Default:
        // Get() function with a default value of dynamically construct an iterator,
        // seek and call the call back function.
        virtual void Get(const LookupKey &k, void *callback_args,
                         bool (*callback_func)(void *arg, const char *entry));

        virtual Status GetAndValidate(const LookupKey &,
                                      void *,
                                      bool (*)(void *arg,
                                               const char *entry),
                                      bool)
        {
            return Status::NotSupported("GetAndValidate() not implemented.");
        }

        virtual uint64_t ApproximateNumEntries(const Slice &,
                                               const Slice &)
        {
            return 0;
        }

        // Returns a vector of unique random memtable entries of approximate
        // size 'target_sample_size' (this size is not strictly enforced).
        virtual void UniqueRandomSample(const uint64_t num_entries,
                                        const uint64_t target_sample_size,
                                        std::unordered_set<const char *> *entries)
        {
            (void)num_entries;
            (void)target_sample_size;
            (void)entries;
            assert(false);
        }

        // Report an approximation of how much memory has been used other than memory
        // that was allocated through the allocator. Safe to call from any thread.
        virtual size_t ApproximateMemoryUsage() = 0;

        virtual ~MemTableRep() {}

        // Iteration over the contents of a skip collection
        class Iterator
        {
        public:
            // Initialize an iterator over the specified collection.
            // The returned iterator is not valid.
            // explicit Iterator(const MemTableRep* collection);
            virtual ~Iterator() {}

            // Returns true iff the iterator is positioned at a valid node.
            virtual bool Valid() const = 0;

            // Returns the key at the current position.
            // REQUIRES: Valid()
            virtual const char *key() const = 0;

            // Advances to the next position.
            // REQUIRES: Valid()
            virtual void Next() = 0;

            // Advances to the next position and performs integrity validations on the
            // skip list. Iterator becomes invalid and Corruption is returned if a
            // corruption is found.
            // REQUIRES: Valid()
            virtual Status NextAndValidate(bool /* allow_data_in_errors */)
            {
                return Status::NotSupported("NextAndValidate() not implemented.");
            }

            // Advances to the previous position.
            // REQUIRES: Valid()
            virtual void Prev() = 0;

            // Advances to the previous position and performs integrity validations on
            // the skip list. Iterator becomes invalid and Corruption is returned if a
            // corruption is found.
            // REQUIRES: Valid()
            virtual Status PrevAndValidate(bool /* allow_data_in_errors */)
            {
                return Status::NotSupported("PrevAndValidate() not implemented.");
            }

            // Advance to the first entry with a key >= target
            virtual void Seek(const Slice &internal_key, const char *memtable_key) = 0;

            // Seek and perform integrity validations on the skip list.
            // Iterator becomes invalid and Corruption is returned if a
            // corruption is found.
            virtual Status SeekAndValidate(const Slice & /* internal_key */,
                                           const char * /* memtable_key */,
                                           bool /* allow_data_in_errors */)
            {
                return Status::NotSupported("SeekAndValidate() not implemented.");
            }

            // retreat to the first entry with a key <= target
            virtual void SeekForPrev(const Slice &internal_key,
                                     const char *memtable_key) = 0;

            virtual void RandomSeek() {}

            // Position at the first entry in collection.
            // Final state of iterator is Valid() iff collection is not empty.
            virtual void SeekToFirst() = 0;

            // Position at the last entry in collection.
            // Final state of iterator is Valid() iff collection is not empty.
            virtual void SeekToLast() = 0;
        };

        // Return an iterator over the keys in this representation.
        // arena: If not null, the arena needs to be used to allocate the Iterator.
        //        When destorying the iterator, the caller will not call "delete"
        //        but Iterator::~Iterator() directly. The destructor needs to destory
        //        all the states but those allocated in arena.
        virtual Iterator *GetIterator(Arena *arena = nullptr) = 0;

        virtual Iterator *GetDynamicPrefixIterator(Arena *arena = nullptr)
        {
            return GetIterator(arena);
        }

        virtual bool IsMergeOperatorSupported() const { return true; }

        virtual bool IsSnapshotSupported() const { return true; }

    protected:
        // When *key is an internal key concatenated with the value, returns the
        // user key.
        virtual Slice UserKey(const char *key) const;

        Allocator *allocator_;
    };

    // This is the base class for all factories that are used by XiaoDB to create
    // new MemTableRep objects
    class MemTableRepFactory : public Customizable
    {
    public:
        ~MemTableRepFactory() override {}

        static const char *Type() { return "MemTableRepFactory"; }
        static Status CreateFromString(const ConfigOptions &config_options,
                                       const std::string &id,
                                       std::unique_ptr<MemTableRepFactory> *factory);
        static Status CreateFromString(const ConfigOptions &config_options,
                                       const std::string &id,
                                       std::shared_ptr<MemTableRepFactory> *factory);

        virtual MemTableRep *CreateMemTableRep(const MemTableRep::KeyComparator &,
                                               Allocator *, const SliceTransform *,
                                               Logger *logger) = 0;
        virtual MemTableRep *CreateMemTableRep(
            const MemTableRep::KeyComparator &key_cmp, Allocator *allocator,
            const SliceTransform *slice_transform, Logger *logger,
            uint32_t)
        {
            return CreateMemTableRep(key_cmp, allocator, slice_transform, logger);
        }

        const char *Name() const override = 0;

        virtual bool IsInsertConcurrentlySupported() const { return false; }

        virtual bool CanHandleDuplicatedKey() const { return false; }
    };

    class SkipListFactory : public MemTableRepFactory
    {
    public:
        explicit SkipListFactory(size_t lookahead = 0);

        static const char *kClassName() { return "SkipListFactory"; }
        static const char *kNickName() { return "skip_list"; }
        const char *Name() const override { return kClassName(); }
        const char *NickName() const override { return kNickName(); }
        std::string GetId() const override;

        using MemTableRepFactory::CreateMemTableRep;
        MemTableRep *CreateMemTableRep(const MemTableRep::KeyComparator &, Allocator *,
                                       const SliceTransform *,
                                       Logger *logger) override;

        bool IsInsertConcurrentlySupported() const override { return true; }

        bool CanHandleDuplicatedKey() const override { return true; }

    private:
        size_t lookahead_;
    };

    // This creates MemTableReps that are backed by an std::vector. On iteration,
    // the vector is sorted. This is useful for workloads where iteration is very
    // rare and writes are generally not issued after reads begin.
    //
    // Parameters:
    //   count: Passed to the constructor of the underlying std::vector of each
    //     VectorRep. On initialization, the underlying array will be at least count
    //     bytes reserved for usage.
    class VectorRepFactory : public MemTableRepFactory
    {
        size_t count_;

    public:
        explicit VectorRepFactory(size_t count = 0);

        // Methods for Configurable/Customizable class overrides
        static const char *kClassName() { return "VectorRepFactory"; }
        static const char *kNickName() { return "vector"; }
        const char *Name() const override { return kClassName(); }
        const char *NickName() const override { return kNickName(); }

        // Methods for MemTableRepFactory class overrides
        using MemTableRepFactory::CreateMemTableRep;
        MemTableRep *CreateMemTableRep(const MemTableRep::KeyComparator &, Allocator *,
                                       const SliceTransform *,
                                       Logger *logger) override;
    };

    MemTableRepFactory *NewHashSkipListRepFactory(
        size_t bucket_count = 1000000, int32_t skiplist_height = 4,
        int32_t skiplist_branching_factor = 4);

    MemTableRepFactory *NewHashLinkListRepFactory(
        size_t bucket_count = 50000, size_t huge_page_tlb_size = 0,
        int bucket_entries_logging_threshould = 4096,
        bool if_log_bucket_dist_when_flash = true,
        uint32_t threshould_use_skiplist = 256);
}