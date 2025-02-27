#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "xiaodb/compression_type.h"
#include "xiaodb/data_structure.h"
#include "xiaodb/memory_allocator.h"

namespace XIAODB_NAMESPACE
{

    class Cache;
    struct ConfigOptions;
    class SecondaryCache;

    // These definitions begin source compatibility for a future change in which
    // a specific class for block cache is split away from general caches, so that
    // the block cache API can continue to become more specialized and
    // customizeable, including in ways incompatible with a general cache. For
    // example, HyperClockCache is not usable as a general cache because it expects
    // only fixed-size block cache keys, but this limitation is not yet reflected
    // in the API function signatures.
    // * Phase 1 (done) - Make both BlockCache and RowCache aliases for Cache,
    // and make a factory function for row caches. Encourage users of row_cache
    // (not common) to switch to the factory function for row caches.
    // * Phase 2 - Split off RowCache as its own class, removing secondary
    // cache support features and more from the API to simplify it. Between Phase 1
    // and Phase 2 users of row_cache will need to update their code. Any time
    // after Phase 2, the block cache and row cache APIs can become more specialized
    // in ways incompatible with general caches.
    // * Phase 3 - Move existing RocksDB uses of Cache to BlockCache, and deprecate
    // (but not yet remove) Cache as an alias for BlockCache.
    using BlockCache = Cache;
    using RowCache = Cache;

    // Classifications of block cache entries.
    //
    // Developer notes: Adding a new enum to this class requires corresponding
    // updates to `kCacheEntryRoleToCamelString` and
    // `kCacheEntryRoleToHyphenString`. Do not add to this enum after `kMisc` since
    // `kNumCacheEntryRoles` assumes `kMisc` comes last.
    enum class CacheEntryRole
    {
        // Block-based table data block
        kDataBlock,
        // Block-based table filter block (full or partitioned)
        kFilterBlock,
        // Block-based table metadata block for partitioned filter
        kFilterMetaBlock,
        // OBSOLETE / DEPRECATED: old/removed block-based filter
        kDeprecatedFilterBlock,
        // Block-based table index block
        kIndexBlock,
        // Other kinds of block-based table block
        kOtherBlock,
        // WriteBufferManager's charge to account for its memtable usage
        kWriteBuffer,
        // Compression dictionary building buffer's charge to account for
        // its memory usage
        kCompressionDictionaryBuildingBuffer,
        // Filter's charge to account for
        // (new) bloom and ribbon filter construction's memory usage
        kFilterConstruction,
        // BlockBasedTableReader's charge to account for its memory usage
        kBlockBasedTableReader,
        // FileMetadata's charge to account for its memory usage
        kFileMetadata,
        // Blob value (when using the same cache as block cache and blob cache)
        kBlobValue,
        // Blob cache's charge to account for its memory usage (when using a
        // separate block cache and blob cache)
        kBlobCache,
        // Default bucket, for miscellaneous cache entries. Do not use for
        // entries that could potentially add up to large usage.
        kMisc,
    };

    constexpr uint32_t kNumCacheEntryRoles =
        static_cast<uint32_t>(CacheEntryRole::kMisc) + 1;

    // Obtain a hyphen-separated, lowercase name of a `CacheEntryRole`.
    const std::string &GetCacheEntryRoleName(CacheEntryRole);

    // A fast bit set for CacheEntryRoles
    using CacheEntryRoleSet = SmallEnumSet<CacheEntryRole, CacheEntryRole::kMisc>;

    // For use with `GetMapProperty()` for property
    // `DB::Properties::kBlockCacheEntryStats`. On success, the map will
    // be populated with all keys that can be obtained from these functions.
    struct BlockCacheEntryStatsMapKeys
    {
        static const std::string &CacheId();
        static const std::string &CacheCapacityBytes();
        static const std::string &LastCollectionDurationSeconds();
        static const std::string &LastCollectionAgeSeconds();

        static std::string EntryCount(CacheEntryRole);
        static std::string UsedBytes(CacheEntryRole);
        static std::string UsedPercent(CacheEntryRole);
    };

    extern const bool kDefaultToAdaptiveMutex;

    enum CacheMetadataChargePolicy
    {
        // Only the `charge` of each entry inserted into a Cache counts against
        // the `capacity`
        kDontChargeCacheMetadata,
        // In addition to the `charge`, the approximate space overheads in the
        // Cache (in bytes) also count against `capacity`. These space overheads
        // are for supporting fast Lookup and managing the lifetime of entries.
        kFullChargeCacheMetadata
    };
    const CacheMetadataChargePolicy kDefaultCacheMetadataChargePolicy =
        kFullChargeCacheMetadata;

    // Options shared betweeen various cache implementations that
    // divide the key space into shards using hashing.
    struct ShardedCacheOptions
    {
        // Capacity of the cache, in the same units as the `charge` of each entry.
        // This is typically measured in bytes, but can be a different unit if using
        // kDontChargeCacheMetadata.
        size_t capacity = 0;

        // Cache is sharded into 2^num_shard_bits shards, by hash of key.
        // If < 0, a good default is chosen based on the capacity and the
        // implementation. (Mutex-based implementations are much more reliant
        // on many shards for parallel scalability.)
        int num_shard_bits = -1;

        // If strict_capacity_limit is set, Insert() will fail if there is not
        // enough capacity for the new entry along with all the existing referenced
        // (pinned) cache entries. (Unreferenced cache entries are evicted as
        // needed, sometimes immediately.) If strict_capacity_limit == false
        // (default), Insert() never fails.
        bool strict_capacity_limit = false;

        // If non-nullptr, RocksDB will use this allocator instead of system
        // allocator when allocating memory for cache blocks.
        //
        // Caveat: when the cache is used as block cache, the memory allocator is
        // ignored when dealing with compression libraries that allocate memory
        // internally (currently only XPRESS).
        std::shared_ptr<MemoryAllocator> memory_allocator;

        // See CacheMetadataChargePolicy
        CacheMetadataChargePolicy metadata_charge_policy =
            kDefaultCacheMetadataChargePolicy;

        // A SecondaryCache instance to use the non-volatile tier. For a RowCache
        // this option must be kept as default empty.
        std::shared_ptr<SecondaryCache> secondary_cache;

        // See hash_seed comments below
        static constexpr int32_t kQuasiRandomHashSeed = -1;
        static constexpr int32_t kHostHashSeed = -2;

        // EXPERT OPTION: Specifies how a hash seed should be determined for the
        // cache, or specifies a specific seed (only recommended for diagnostics or
        // testing).
        //
        // Background: it could be dangerous to have different cache instances
        // access the same SST files with the same hash seed, as correlated unlucky
        // hashing across hosts or restarts could cause a widespread issue, rather
        // than an isolated one. For example, with smaller block caches, it is
        // possible for large full Bloom filters in a set of SST files to be randomly
        // clustered into one cache shard, causing mutex contention or a thrashing
        // condition as there's little or no space left for other entries assigned to
        // the shard. If a set of SST files is broadcast and used on many hosts, we
        // should ensure all have an independent chance of balanced shards.
        //
        // Values >= 0 will be treated as fixed hash seeds. Values < 0 are reserved
        // for methods of dynamically choosing a seed, currently:
        // * kQuasiRandomHashSeed - Each cache created chooses a seed mostly randomly,
        //   except that within a process, no seed is repeated until all have been
        //   issued.
        // * kHostHashSeed - The seed is determined based on hashing the host name.
        //   Although this is arguably slightly worse for production reliability, it
        //   solves the essential problem of cross-host correlation while ensuring
        //   repeatable behavior on a host, for diagnostic purposes.
        int32_t hash_seed = kHostHashSeed;

        ShardedCacheOptions() {}
        ShardedCacheOptions(
            size_t _capacity, int _num_shard_bits, bool _strict_capacity_limit,
            std::shared_ptr<MemoryAllocator> _memory_allocator = nullptr,
            CacheMetadataChargePolicy _metadata_charge_policy =
                kDefaultCacheMetadataChargePolicy)
            : capacity(_capacity),
              num_shard_bits(_num_shard_bits),
              strict_capacity_limit(_strict_capacity_limit),
              memory_allocator(std::move(_memory_allocator)),
              metadata_charge_policy(_metadata_charge_policy) {}
        // Make ShardedCacheOptions polymorphic
        virtual ~ShardedCacheOptions() = default;
    };

    struct LRUCacheOptions : public ShardedCacheOptions
    {
        double high_pri_pool_ratio = 0.5;
        double low_pri_pool_ratio = 0.0;

        bool use_adaptive_mutex = kDefaultToAdaptiveMutex;

        LRUCacheOptions() {}
        LRUCacheOptions(size_t _capacity, int _num_shard_bits,
                        bool _strict_capacity_limit, double _high_pri_pool_ratio,
                        std::shared_ptr<MemoryAllocator> _memory_allocator = nullptr,
                        bool _use_adaptive_mutex = kDefaultToAdaptiveMutex,
                        CacheMetadataChargePolicy _metadata_charge_policy =
                            kDefaultCacheMetadataChargePolicy,
                        double _low_pri_pool_ratio = 0.0)
            : ShardedCacheOptions(_capacity, _num_shard_bits, _strict_capacity_limit,
                                  std::move(_memory_allocator),
                                  _metadata_charge_policy),
              high_pri_pool_ratio(_high_pri_pool_ratio),
              low_pri_pool_ratio(_low_pri_pool_ratio),
              use_adaptive_mutex(_use_adaptive_mutex) {}

        std::shared_ptr<Cache> MakeSharedCache() const;

        std::shared_ptr<RowCache> MakeSharedCache() const;
    };

    inline std::shared_ptr<Cache> NewLRUCache(
        size_t capacity, int num_shard_bits = -1,
        bool strict_capacity_limit = false, double high_pri_pool_ratio = 0.5,
        std::shared_ptr<MemoryAllocator> memory_allocator = nullptr,
        bool use_adaptive_mutex = kDefaultToAdaptiveMutex,
        CacheMetadataChargePolicy metadata_charge_policy =
            kDefaultCacheMetadataChargePolicy,
        double low_pri_pool_ratio = 0.0)
    {
        return LRUCacheOptions(capacity, num_shard_bits, strict_capacity_limit,
                               high_pri_pool_ratio, memory_allocator,
                               use_adaptive_mutex, metadata_charge_policy,
                               low_pri_pool_ratio)
            .MakeSharedCache();
    }

    inline std::shared_ptr<Cache> NewLRUCache(const LRUCacheOptions &cache_opts)
    {
        return cache_opts.MakeSharedCache();
    }

    struct CompressedSecondaryCacheOptions : LRUCacheOptions
    {
        // The compression method (if any) that is used to compress data.
        CompressionType compression_type = CompressionType::kLZ4Compression;

        // Options specific to the compression algorithm
        CompressionOptions compression_opts;

        // compress_format_version can have two values:
        // compress_format_version == 1 -- decompressed size is not included in the
        // block header.
        // compress_format_version == 2 -- decompressed size is included in the block
        // header in varint32 format.
        uint32_t compress_format_version = 2;

        // Enable the custom split and merge feature, which split the compressed value
        // into chunks so that they may better fit jemalloc bins.
        bool enable_custom_split_merge = false;

        // Kinds of entries that should not be compressed, but can be stored.
        // (Filter blocks are essentially non-compressible but others usually are.)
        CacheEntryRoleSet do_not_compress_roles = {CacheEntryRole::kFilterBlock};

        CompressedSecondaryCacheOptions() {}
        CompressedSecondaryCacheOptions(
            size_t _capacity, int _num_shard_bits, bool _strict_capacity_limit,
            double _high_pri_pool_ratio, double _low_pri_pool_ratio = 0.0,
            std::shared_ptr<MemoryAllocator> _memory_allocator = nullptr,
            bool _use_adaptive_mutex = kDefaultToAdaptiveMutex,
            CacheMetadataChargePolicy _metadata_charge_policy =
                kDefaultCacheMetadataChargePolicy,
            CompressionType _compression_type = CompressionType::kLZ4Compression,
            uint32_t _compress_format_version = 2,
            bool _enable_custom_split_merge = false,
            const CacheEntryRoleSet &_do_not_compress_roles =
                {CacheEntryRole::kFilterBlock})
            : LRUCacheOptions(_capacity, _num_shard_bits, _strict_capacity_limit,
                              _high_pri_pool_ratio, std::move(_memory_allocator),
                              _use_adaptive_mutex, _metadata_charge_policy,
                              _low_pri_pool_ratio),
              compression_type(_compression_type),
              compress_format_version(_compress_format_version),
              enable_custom_split_merge(_enable_custom_split_merge),
              do_not_compress_roles(_do_not_compress_roles) {}

        // Construct an instance of CompressedSecondaryCache using these options
        std::shared_ptr<SecondaryCache> MakeSharedSecondaryCache() const;

        // Avoid confusion with LRUCache
        std::shared_ptr<Cache> MakeSharedCache() const = delete;
    };

    // DEPRECATED wrapper function
    inline std::shared_ptr<SecondaryCache> NewCompressedSecondaryCache(
        size_t capacity, int num_shard_bits = -1,
        bool strict_capacity_limit = false, double high_pri_pool_ratio = 0.5,
        double low_pri_pool_ratio = 0.0,
        std::shared_ptr<MemoryAllocator> memory_allocator = nullptr,
        bool use_adaptive_mutex = kDefaultToAdaptiveMutex,
        CacheMetadataChargePolicy metadata_charge_policy =
            kDefaultCacheMetadataChargePolicy,
        CompressionType compression_type = CompressionType::kLZ4Compression,
        uint32_t compress_format_version = 2,
        bool enable_custom_split_merge = false,
        const CacheEntryRoleSet &_do_not_compress_roles = {
            CacheEntryRole::kFilterBlock})
    {
        return CompressedSecondaryCacheOptions(
                   capacity, num_shard_bits, strict_capacity_limit,
                   high_pri_pool_ratio, low_pri_pool_ratio, memory_allocator,
                   use_adaptive_mutex, metadata_charge_policy, compression_type,
                   compress_format_version, enable_custom_split_merge,
                   _do_not_compress_roles)
            .MakeSharedSecondaryCache();
    }

    // DEPRECATED wrapper function
    inline std::shared_ptr<SecondaryCache> NewCompressedSecondaryCache(
        const CompressedSecondaryCacheOptions &opts)
    {
        return opts.MakeSharedSecondaryCache();
    }

    struct HyperClockCacheOptions : public ShardedCacheOptions
    {
        // The estimated average `charge` associated with cache entries.
        //
        // EXPERIMENTAL: the field can be set to 0 to size the table dynamically
        // and automatically. See also min_avg_entry_charge. This feature requires
        // platform support for lazy anonymous memory mappings (incl Linux, Windows).
        // Performance is very similar to choosing the best configuration parameter.
        //
        // PRODUCTION-TESTED: This is a critical configuration parameter for good
        // performance, because having a table size that is fixed at creation time
        // greatly reduces the required synchronization between threads.
        // * If the estimate is substantially too low (e.g. less than half the true
        // average) then metadata space overhead with be substantially higher (e.g.
        // 200 bytes per entry rather than 100). With kFullChargeCacheMetadata, this
        // can slightly reduce cache hit rates, and slightly reduce access times due
        // to the larger working memory size.
        // * If the estimate is substantially too high (e.g. 25% higher than the true
        // average) then there might not be sufficient slots in the hash table for
        // both efficient operation and capacity utilization (hit rate). The hyper
        // cache will evict entries to prevent load factors that could dramatically
        // affect lookup times, instead letting the hit rate suffer by not utilizing
        // the full capacity.
        //
        // A reasonable choice is the larger of block_size and metadata_block_size.
        // When WriteBufferManager (and similar) charge memory usage to the block
        // cache, this can lead to the same effect as estimate being too low, which
        // is better than the opposite. Therefore, the general recommendation is to
        // assume that other memory charged to block cache could be negligible, and
        // ignore it in making the estimate.
        //
        // The best parameter choice based on a cache in use is given by
        // GetUsage() / GetOccupancyCount(), ignoring metadata overheads such as
        // with kDontChargeCacheMetadata. More precisely with
        // kFullChargeCacheMetadata is (GetUsage() - 64 * GetTableAddressCount()) /
        // GetOccupancyCount(). However, when the average value size might vary
        // (e.g. balance between metadata and data blocks in cache), it is better
        // to estimate toward the lower side than the higher side.
        size_t estimated_entry_charge;

        // EXPERIMENTAL: When estimated_entry_charge == 0, this parameter establishes
        // a promised lower bound on the average charge of all entries in the table,
        // which is roughly the average uncompressed SST block size of block cache
        // entries, typically > 4KB. The default should generally suffice with almost
        // no cost. (This option is ignored for estimated_entry_charge > 0.)
        //
        // More detail: The table for indexing cache entries will grow automatically
        // as needed, but a hard upper bound on that size is needed at creation time.
        // The reason is that a contiguous memory mapping for the maximum size is
        // created, but memory pages are only mapped to physical (RSS) memory as
        // needed. If the average charge of all entries in the table falls below
        // this value, the table will operate below its full logical capacity (total
        // memory usage) because it has reached its physical capacity for efficiently
        // indexing entries. The hash table is never allowed to exceed a certain safe
        // load factor for efficient Lookup, Insert, etc.
        size_t min_avg_entry_charge = 450;

        // A tuning parameter to cap eviction CPU usage in a "thrashing" situation
        // by allowing the memory capacity to be exceeded slightly as needed. The
        // default setting should offer balanced protection against excessive CPU
        // and memory usage under extreme stress conditions, with no effect on
        // normal operation. Such stress conditions are proportionally more likely
        // with small caches (10s of MB or less) vs. large caches (GB-scale).
        // (NOTE: With the unusual setting of strict_capacity_limit=true, this
        // parameter is ignored.)
        //
        // BACKGROUND: Without some kind of limiter, inserting into a CLOCK-based
        // cache with no evictable entries (all "pinned") requires scanning the
        // entire cache to determine that nothing can be evicted. (By contrast,
        // LRU caches can determine no entries are evictable in O(1) time, but
        // require more synchronization/coordination on that eviction metadata.)
        // This aspect of a CLOCK cache can make a stressed situation worse by
        // bogging down the CPU with repeated scans of the cache. And with
        // strict_capacity_limit=false (normal setting), finding something evictable
        // doesn't change the outcome of insertion: the entry is inserted anyway
        // and the cache is allowed to exceed its target capacity if necessary.
        //
        // SOLUTION: Eviction is aborted upon seeing some number of pinned
        // entries before evicting anything, or if the ratio of pinned to evicted
        // is too high. This setting `eviction_effort_cap` essentially controls both
        // that allowed initial number of pinned entries and the maximum allowed
        // ratio. As the pinned size approaches the target cache capacity, roughly
        // 1/eviction_effort_cap additional portion of the capacity might be kept
        // in memory and evictable in order to keep CLOCK eviction reasonably
        // performant. Under the default setting and high stress conditions, this
        // memory overhead is around 3-5%. Under normal or even moderate stress
        // conditions, the memory overhead is negligible to zero.
        //
        // A large value like 1000 offers some protection with essentially no
        // memory overhead, while the minimum value of 1 could be useful for a
        // small cache where roughly doubling in size under stress could be OK to
        // keep operations very fast.
        int eviction_effort_cap = 30;

        HyperClockCacheOptions(
            size_t _capacity, size_t _estimated_entry_charge,
            int _num_shard_bits = -1, bool _strict_capacity_limit = false,
            std::shared_ptr<MemoryAllocator> _memory_allocator = nullptr,
            CacheMetadataChargePolicy _metadata_charge_policy =
                kDefaultCacheMetadataChargePolicy)
            : ShardedCacheOptions(_capacity, _num_shard_bits, _strict_capacity_limit,
                                  std::move(_memory_allocator),
                                  _metadata_charge_policy),
              estimated_entry_charge(_estimated_entry_charge) {}

        // Construct an instance of HyperClockCache using these options
        std::shared_ptr<Cache> MakeSharedCache() const;
    };

    // DEPRECATED - The old Clock Cache implementation had an unresolved bug and
    // has been removed. The new HyperClockCache requires an additional
    // configuration parameter that is not provided by this API. This function
    // simply returns a new LRUCache for functional compatibility.
    std::shared_ptr<Cache> NewClockCache(
        size_t capacity, int num_shard_bits = -1,
        bool strict_capacity_limit = false,
        CacheMetadataChargePolicy metadata_charge_policy =
            kDefaultCacheMetadataChargePolicy);

    enum PrimaryCacheType
    {
        kCacheTypeLRU, // LRU cache type
        kCacheTypeHCC, // Hyper Clock Cache type
        kCacheTypeMax,
    };

    enum TieredAdmissionPolicy
    {
        // Automatically select the admission policy
        kAdmPolicyAuto,
        // During promotion/demotion, first time insert a placeholder entry, second
        // time insert the full entry if the placeholder is found, i.e insert on
        // second hit
        kAdmPolicyPlaceholder,
        // Same as kAdmPolicyPlaceholder, but also if an entry in the primary cache
        // was a hit, then force insert it into the compressed secondary cache
        kAdmPolicyAllowCacheHits,
        // An admission policy for three cache tiers - primary uncompressed,
        // compressed secondary, and a compressed local flash (non-volatile) cache.
        // Each tier is managed as an independent queue.
        kAdmPolicyThreeQueue,
        // Allow all blocks evicted from the primary block cache into the secondary
        // cache. This may increase CPU overhead due to more blocks being admitted
        // and compressed, but may increase the compressed secondary cache hit rate
        // for some workloads
        kAdmPolicyAllowAll,
        kAdmPolicyMax,
    };

    // EXPERIMENTAL
    // The following feature is experimental, and the API is subject to change
    //
    // A 2-tier cache with a primary block cache, and a compressed secondary
    // cache. The returned cache instance will internally allocate a primary
    // uncompressed cache of the specified type, and a compressed secondary
    // cache. Any cache memory reservations, such as WriteBufferManager
    // allocations costed to the block cache, will be distributed
    // proportionally across both the primary and secondary.
    struct TieredCacheOptions
    {
        // This should point to an instance of either LRUCacheOptions or
        // HyperClockCacheOptions, depending on the cache_type. In either
        // case, the capacity and secondary_cache fields in those options
        // should not be set. If set, they will be ignored by NewTieredCache.
        ShardedCacheOptions *cache_opts = nullptr;
        PrimaryCacheType cache_type = PrimaryCacheType::kCacheTypeLRU;
        TieredAdmissionPolicy adm_policy = TieredAdmissionPolicy::kAdmPolicyAuto;
        CompressedSecondaryCacheOptions comp_cache_opts;
        // Any capacity specified in LRUCacheOptions, HyperClockCacheOptions and
        // CompressedSecondaryCacheOptions is ignored
        // The total_capacity specified here is taken as the memory budget and
        // divided between the primary block cache and compressed secondary cache
        size_t total_capacity = 0;
        double compressed_secondary_ratio = 0.0;
        // An optional secondary cache that will serve as the persistent cache
        // tier. If present, compressed blocks will be written to this
        // secondary cache.
        std::shared_ptr<SecondaryCache> nvm_sec_cache;
    };

    std::shared_ptr<Cache> NewTieredCache(const TieredCacheOptions &cache_opts);

    // EXPERIMENTAL
    // Dynamically update some of the parameters of a TieredCache. The input
    // cache shared_ptr should have been allocated using NewTieredVolatileCache.
    // At the moment, there are a couple of limitations -
    // 1. The total_capacity should be > the WriteBufferManager max size, if
    //    using the block cache charging feature
    // 2. Once the compressed secondary cache is disabled by setting the
    //    compressed_secondary_ratio to 0.0, it cannot be dynamically re-enabled
    //    again
    Status UpdateTieredCache(
        const std::shared_ptr<Cache> &cache, int64_t total_capacity = -1,
        double compressed_secondary_ratio = std::numeric_limits<double>::max(),
        TieredAdmissionPolicy adm_policy = TieredAdmissionPolicy::kAdmPolicyMax);

}