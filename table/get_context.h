#pragma once

#include <string>

#include "db/read_callback.h"
#include "xiaodb/types.h"

namespace XIAODB_NAMESPACE
{
    class BlobFetcher;
    class Comparator;
    class Logger;
    class MergeContext;
    class MergeOperator;
    class PinnableWideColumns;
    class PinnedIteratorsManager;
    class Statistics;
    class SystemClock;
    struct ParsedInternalKey;

    /**
     * @brief Data structure for accumulating statistics during a point lookup. At the
     * end of the point lookup, the corresponding ticker stats are updated. This
     * avoids the overhead of frequent ticker stats updates
     *
     */
    struct GetContextStats
    {
        uint64_t num_cache_hit = 0;
        uint64_t num_cache_index_hit = 0;
        uint64_t num_cache_data_hit = 0;
        uint64_t num_cache_filter_hit = 0;
        uint64_t num_cache_compression_dict_hit = 0;
        uint64_t num_cache_index_miss = 0;
        uint64_t num_cache_filter_miss = 0;
        uint64_t num_cache_data_miss = 0;
        uint64_t num_cache_compression_dict_miss = 0;
        uint64_t num_cache_bytes_read = 0;
        uint64_t num_cache_miss = 0;
        uint64_t num_cache_add = 0;
        uint64_t num_cache_add_redundant = 0;
        uint64_t num_cache_bytes_write = 0;
        uint64_t num_cache_index_add = 0;
        uint64_t num_cache_index_add_redundant = 0;
        uint64_t num_cache_index_bytes_insert = 0;
        uint64_t num_cache_data_add = 0;
        uint64_t num_cache_data_add_redundant = 0;
        uint64_t num_cache_data_bytes_insert = 0;
        uint64_t num_cache_filter_add = 0;
        uint64_t num_cache_filter_add_redundant = 0;
        uint64_t num_cache_filter_bytes_insert = 0;
        uint64_t num_cache_compression_dict_add = 0;
        uint64_t num_cache_compression_dict_add_redundant = 0;
        uint64_t num_cache_compression_dict_bytes_insert = 0;
        // MultiGet stats.
        uint64_t num_filter_read = 0;
        uint64_t num_index_read = 0;
        uint64_t num_sst_read = 0;
    };

    class GetContext
    {
    public:
        enum GetState
        {
            kNotFound,
            kFound,
            kDeleted,
            kCorrupt,
            kMerge,
            kUnexpectedBlobIndex,
            kMergeOperatorFailed,
        };

        GetContextStats get_context_stats_;

        private:
        // Helper method that postprocesses the results of merge operations, e.g. it
        // sets the state correctly upon merge errors.
        void PostprocessMerge(const Status &merge_status);

        // The following methods perform the actual merge operation for the
        // no base value/plain base value/wide-column base value cases.
        void MergeWithNoBaseValue();
        void MergeWithPlainBaseValue(const Slice &value);
        void MergeWithWideColumnBaseValue(const Slice &entity);

        bool GetBlobValue(const Slice &user_key, const Slice &blob_index,
                          PinnableSlice *blob_value, Status *read_status);

        void appendToReplayLog(ValueType type, Slice value, Slice ts);

        const Comparator *ucmp_;
        const MergeOperator *merge_operator_;
        // the merge operations encountered;
        Logger *logger_;
        Statistics *statistics_;

        GetState state_;
        Slice user_key_;
        // When a blob index is found with the user key containing timestamp,
        // this copies the corresponding user key on record in the sst file
        // and is later used for blob verification.
        PinnableSlice ukey_with_ts_found_;
        PinnableSlice *pinnable_val_;
        PinnableWideColumns *columns_;
        std::string *timestamp_;
        bool ts_from_rangetombstone_{false};
        bool *value_found_; // Is value set correctly? Used by KeyMayExist
        MergeContext *merge_context_;
        SequenceNumber *max_covering_tombstone_seq_;
        SystemClock *clock_;
        // If a key is found, seq_ will be set to the SequenceNumber of most recent
        // write to the key or kMaxSequenceNumber if unknown
        SequenceNumber *seq_;
        std::string *replay_log_;
        // Used to temporarily pin blocks when state_ == GetContext::kMerge
        PinnedIteratorsManager *pinned_iters_mgr_;
        ReadCallback *callback_;
        bool sample_;
        // Value is true if it's called as part of DB Get API and false if it's
        // called as part of DB GetMergeOperands API. When it's false merge operators
        // are never merged.
        bool do_merge_;
        bool *is_blob_index_;
        // Used for block cache tracing only. A tracing get id uniquely identifies a
        // Get or a MultiGet.
        const uint64_t tracing_get_id_;
        BlobFetcher *blob_fetcher_;
    };
}