#pragma once

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "xiaodb/advanced_options.h"
#include "xiaodb/comparator.h"
#include "xiaodb/compression_type.h"
#include "xiaodb/customizable.h"
#include "xiaodb/data_structure.h"
#include "xiaodb/env.h"
#include "xiaodb/file_checksum.h"
#include "xiaodb/listener.h"
#include "xiaodb/sst_partitioner.h"
#include "xiaodb/types.h"
#include "xiaodb/universal_compaction.h"
#include "xiaodb/version.h"
#include "xiaodb/write_buffer_manager.h"

#ifdef max
#undef max
#endif

namespace XIAODB_NAMESPACE
{

    class Cache;
    class CompactionFilter;
    class CompactionFilterFactory;
    class Comparator;
    class ConcurrentTaskLimiter;
    class Env;
    enum InfoLogLevel : unsigned char;
    class SstFileManager;
    class FilterPolicy;
    class Logger;
    class MergeOperator;
    class Snapshot;
    class MemTableRepFactory;
    class RateLimiter;
    class Slice;
    class Statistics;
    class InternalKeyComparator;
    class WalFilter;
    class FileSystem;

    struct Options;
    struct DbPath;

    using FileTypeSet = SmallEnumSet<FileType, FileType::kBlobFile>;

    struct ColumnFamilyOptions : public AdvancedColumnFamilyOptions
    {
    };

    struct DBOptions
    {
        // The function recovers options to the option as in version 4.6.
        // NOT MAINTAINED: This function has not been and is not maintained.
        // DEPRECATED: This function might be removed in a future release.
        // In general, defaults are changed to suit broad interests. Opting
        // out of a change on upgrade should be deliberate and considered.
        DBOptions *OldDefaults(int rocksdb_major_version = 4,
                               int rocksdb_minor_version = 6);

        // Some functions that make it easier to optimize RocksDB

        // Use this if your DB is very small (like under 1GB) and you don't want to
        // spend lots of memory for memtables.
        // An optional cache object is passed in for the memory of the
        // memtable to cost to
        DBOptions *OptimizeForSmallDb(std::shared_ptr<Cache> *cache = nullptr);

        // By default, RocksDB uses only one background thread for flush and
        // compaction. Calling this function will set it up such that total of
        // `total_threads` is used. Good value for `total_threads` is the number of
        // cores. You almost definitely want to call this function if your system is
        // bottlenecked by RocksDB.
        DBOptions *IncreaseParallelism(int total_threads = 16);

        // If true, the database will be created if it is missing.
        // Default: false
        bool create_if_missing = false;

        // If true, missing column families will be automatically created on
        // DB::Open().
        // Default: false
        bool create_missing_column_families = false;

        // If true, an error is raised if the database already exists.
        // Default: false
        bool error_if_exists = false;

        // If true, RocksDB does some pro-active and generally inexpensive checks
        // for DB or data corruption, on top of usual protections such as block
        // checksums. True also enters a read-only mode when a DB write fails;
        // see DB::Resume().
        //
        // As most workloads value data correctness over availability, this option
        // is on by default. Note that the name of this old option is potentially
        // misleading, and other options and operations go further in proactive
        // checking for corruption, including
        // * paranoid_file_checks
        // * paranoid_memory_checks
        // * DB::VerifyChecksum()
        //
        // Default: true
        bool paranoid_checks = true;

        // DEPRECATED: This option might be removed in a future release.
        //
        // If true, during memtable flush, RocksDB will validate total entries
        // read in flush, and compare with counter inserted into it.
        //
        // The option is here to turn the feature off in case this new validation
        // feature has a bug. The option may be removed in the future once the
        // feature is stable.
        //
        // Default: true
        bool flush_verify_memtable_count = true;

        // DEPRECATED: This option might be removed in a future release.
        //
        // If true, during compaction, RocksDB will count the number of entries
        // read and compare it against the number of entries in the compaction
        // input files. This is intended to add protection against corruption
        // during compaction. Note that
        // - this verification is not done for compactions during which a compaction
        // filter returns kRemoveAndSkipUntil, and
        // - the number of range deletions is not verified.
        //
        // The option is here to turn the feature off in case this new validation
        // feature has a bug. The option may be removed in the future once the
        // feature is stable.
        //
        // Default: true
        bool compaction_verify_record_count = true;

        // If true, the log numbers and sizes of the synced WALs are tracked
        // in MANIFEST. During DB recovery, if a synced WAL is missing
        // from disk, or the WAL's size does not match the recorded size in
        // MANIFEST, an error will be reported and the recovery will be aborted.
        //
        // This is one additional protection against WAL corruption besides the
        // per-WAL-entry checksum.
        //
        // Note that this option does not work with secondary instance.
        // Currently, only syncing closed WALs are tracked. Calling `DB::SyncWAL()`,
        // etc. or writing with `WriteOptions::sync=true` to sync the live WAL is not
        // tracked for performance/efficiency reasons.
        //
        // Default: false
        bool track_and_verify_wals_in_manifest = false;

        // EXPERIMENTAL
        //
        // If true, each new WAL will record various information about its predecessor
        // WAL for verification on the predecessor WAL during WAL recovery.
        //
        // It verifies the following:
        // 1. There exists at least some WAL in the DB
        // - It's not compatible with `RepairDB()` since this option imposes a
        // stricter requirement on WAL than the DB went through `RepariDB()` can
        // normally meet
        // 2. There exists no WAL hole where new WAL data presents while some old WAL
        // data not yet obsolete is missing. The DB manifest indicates which WALs are
        // obsolete.
        //
        // This is intended to be a better replacement to
        // `track_and_verify_wals_in_manifest`.
        //
        // Default: false
        bool track_and_verify_wals = false;

        // If true, verifies the SST unique id between MANIFEST and actual file
        // each time an SST file is opened. This check ensures an SST file is not
        // overwritten or misplaced. A corruption error will be reported if mismatch
        // detected, but only when MANIFEST tracks the unique id, which starts from
        // RocksDB version 7.3. Although the tracked internal unique id is related
        // to the one returned by GetUniqueIdFromTableProperties, that is subject to
        // change.
        // NOTE: verification is currently only done on SST files using block-based
        // table format.
        //
        // Setting to false should only be needed in case of unexpected problems.
        //
        // Although an early version of this option opened all SST files for
        // verification on DB::Open, that is no longer guaranteed. However, as
        // documented in an above option, if max_open_files is -1, DB will open all
        // files on DB::Open().
        //
        // Default: true
        bool verify_sst_unique_id_in_manifest = true;

        // Use the specified object to interact with the environment,
        // e.g. to read/write files, schedule background work, etc.
        // Default: Env::Default()
        Env *env = Env::Default();

        // Limits internal file read/write bandwidth:
        //
        // - Flush requests write bandwidth at `Env::IOPriority::IO_HIGH`
        // - Compaction requests read and write bandwidth at
        //   `Env::IOPriority::IO_LOW`
        // - Reads associated with a `ReadOptions` can be charged at
        //   `ReadOptions::rate_limiter_priority` (see that option's API doc for usage
        //   and limitations).
        // - Writes associated with a `WriteOptions` can be charged at
        //   `WriteOptions::rate_limiter_priority` (see that option's API doc for
        //   usage and limitations).
        //
        // Rate limiting is disabled if nullptr. If rate limiter is enabled,
        // bytes_per_sync is set to 1MB by default.
        //
        // Default: nullptr
        std::shared_ptr<RateLimiter> rate_limiter = nullptr;

        // Use to track SST files and control their file deletion rate.
        //
        // Features:
        //  - Throttle the deletion rate of the SST files.
        //  - Keep track the total size of all SST files.
        //  - Set a maximum allowed space limit for SST files that when reached
        //    the DB wont do any further flushes or compactions and will set the
        //    background error.
        //  - Can be shared between multiple dbs.
        // Limitations:
        //  - Only track and throttle deletes of SST files in
        //    first db_path (db_name if db_paths is empty).
        //
        // Default: nullptr
        std::shared_ptr<SstFileManager> sst_file_manager = nullptr;

        // Any internal progress/error information generated by the db will
        // be written to info_log if it is non-nullptr, or to a file stored
        // in the same directory as the DB contents if info_log is nullptr.
        // Default: nullptr
        std::shared_ptr<Logger> info_log = nullptr;

        // Minimum level for sending log messages to info_log. The default is
        // INFO_LEVEL when RocksDB is compiled in release mode, and DEBUG_LEVEL
        // when it is compiled in debug mode.
        InfoLogLevel info_log_level = Logger::kDefaultLogLevel;

        // Number of open files that can be used by the DB.  You may need to
        // increase this if your database has a large working set. Value -1 means
        // files opened are always kept open. You can estimate number of files based
        // on target_file_size_base and target_file_size_multiplier for level-based
        // compaction. For universal-style compaction, you can usually set it to -1.
        //
        // A high value or -1 for this option can cause high memory usage.
        // See BlockBasedTableOptions::cache_usage_options to constrain
        // memory usage in case of block based table format.
        //
        // Default: -1
        //
        // Dynamically changeable through SetDBOptions() API.
        int max_open_files = -1;

        // If max_open_files is -1, DB will open all files on DB::Open(). You can
        // use this option to increase the number of threads used to open the files.
        // Default: 16
        int max_file_opening_threads = 16;

        // Once write-ahead logs exceed this size, we will start forcing the flush of
        // column families whose memtables are backed by the oldest live WAL file
        // (i.e. the ones that are causing all the space amplification). If set to 0
        // (default), we will dynamically choose the WAL size limit to be
        // [sum of all write_buffer_size * max_write_buffer_number] * 4
        //
        // For example, with 15 column families, each with
        // write_buffer_size = 128 MB
        // max_write_buffer_number = 6
        // max_total_wal_size will be calculated to be [15 * 128MB * 6] * 4 = 45GB
        //
        // The RocksDB wiki has some discussion about how the WAL interacts
        // with memtables and flushing of column families.
        // https://github.com/facebook/rocksdb/wiki/Column-Families
        //
        // This option takes effect only when there are more than one column
        // family as otherwise the wal size is dictated by the write_buffer_size.
        //
        // Default: 0
        //
        // Dynamically changeable through SetDBOptions() API.
        uint64_t max_total_wal_size = 0;

        // If non-null, then we should collect metrics about database operations
        std::shared_ptr<Statistics> statistics = nullptr;

        // By default, writes to stable storage use fdatasync (on platforms
        // where this function is available). If this option is true,
        // fsync is used instead.
        //
        // fsync and fdatasync are equally safe for our purposes and fdatasync is
        // faster, so it is rarely necessary to set this option. It is provided
        // as a workaround for kernel/filesystem bugs, such as one that affected
        // fdatasync with ext4 in kernel versions prior to 3.7.
        bool use_fsync = false;

        // A list of paths where SST files can be put into, with its target size.
        // Newer data is placed into paths specified earlier in the vector while
        // older data gradually moves to paths specified later in the vector.
        //
        // For example, you have a flash device with 10GB allocated for the DB,
        // as well as a hard drive of 2TB, you should config it to be:
        //   [{"/flash_path", 10GB}, {"/hard_drive", 2TB}]
        //
        // The system will try to guarantee data under each path is close to but
        // not larger than the target size. But current and future file sizes used
        // by determining where to place a file are based on best-effort estimation,
        // which means there is a chance that the actual size under the directory
        // is slightly more than target size under some workloads. User should give
        // some buffer room for those cases.
        //
        // If none of the paths has sufficient room to place a file, the file will
        // be placed to the last path anyway, despite to the target size.
        //
        // Placing newer data to earlier paths is also best-efforts. User should
        // expect user files to be placed in higher levels in some extreme cases.
        //
        // If left empty, only one path will be used, which is db_name passed when
        // opening the DB.
        // Default: empty
        std::vector<DbPath> db_paths;

        // This specifies the info LOG dir.
        // If it is empty, the log files will be in the same dir as data.
        // If it is non empty, the log files will be in the specified dir,
        // and the db data dir's absolute path will be used as the log file
        // name's prefix.
        std::string db_log_dir = "";

        // This specifies the absolute dir path for write-ahead logs (WAL).
        // If it is empty, the log files will be in the same dir as data,
        //   dbname is used as the data dir by default
        // If it is non empty, the log files will be in kept the specified dir.
        // When destroying the db,
        //   all log files in wal_dir and the dir itself is deleted
        std::string wal_dir = "";

        // The periodicity when obsolete files get deleted. The default
        // value is 6 hours. The files that get out of scope by compaction
        // process will still get automatically delete on every compaction,
        // regardless of this setting
        //
        // Default: 6 hours
        //
        // Dynamically changeable through SetDBOptions() API.
        uint64_t delete_obsolete_files_period_micros = 6ULL * 60 * 60 * 1000000;

        // Maximum number of concurrent background jobs (compactions and flushes).
        //
        // Default: 2
        //
        // Dynamically changeable through SetDBOptions() API.
        int max_background_jobs = 2;

        // DEPRECATED: RocksDB automatically decides this based on the
        // value of max_background_jobs. For backwards compatibility we will set
        // `max_background_jobs = max_background_compactions + max_background_flushes`
        // in the case where user sets at least one of `max_background_compactions` or
        // `max_background_flushes` (we replace -1 by 1 in case one option is unset).
        //
        // Maximum number of concurrent background compaction jobs, submitted to
        // the default LOW priority thread pool.
        //
        // If you're increasing this, also consider increasing number of threads in
        // LOW priority thread pool. For more information, see
        // Env::SetBackgroundThreads
        //
        // Default: -1
        //
        // Dynamically changeable through SetDBOptions() API.
        int max_background_compactions = -1;

        // This value represents the maximum number of threads that will
        // concurrently perform a compaction job by breaking it into multiple,
        // smaller ones that are run simultaneously.
        // Default: 1 (i.e. no subcompactions)
        //
        // Dynamically changeable through SetDBOptions() API.
        uint32_t max_subcompactions = 1;

        // DEPRECATED: RocksDB automatically decides this based on the
        // value of max_background_jobs. For backwards compatibility we will set
        // `max_background_jobs = max_background_compactions + max_background_flushes`
        // in the case where user sets at least one of `max_background_compactions` or
        // `max_background_flushes`.
        //
        // Maximum number of concurrent background memtable flush jobs, submitted by
        // default to the HIGH priority thread pool. If the HIGH priority thread pool
        // is configured to have zero threads, flush jobs will share the LOW priority
        // thread pool with compaction jobs.
        //
        // It is important to use both thread pools when the same Env is shared by
        // multiple db instances. Without a separate pool, long running compaction
        // jobs could potentially block memtable flush jobs of other db instances,
        // leading to unnecessary Put stalls.
        //
        // If you're increasing this, also consider increasing number of threads in
        // HIGH priority thread pool. For more information, see
        // Env::SetBackgroundThreads
        // Default: -1
        int max_background_flushes = -1;

        // Specify the maximal size of the info log file. If the log file
        // is larger than `max_log_file_size`, a new info log file will
        // be created.
        // If max_log_file_size == 0, all logs will be written to one
        // log file.
        size_t max_log_file_size = 0;

        // Time for the info log file to roll (in seconds).
        // If specified with non-zero value, log file will be rolled
        // if it has been active longer than `log_file_time_to_roll`.
        // Default: 0 (disabled)
        size_t log_file_time_to_roll = 0;

        // Maximal info log files to be kept.
        // Default: 1000
        size_t keep_log_file_num = 1000;

        // Recycle log files.
        // If non-zero, we will reuse previously written log files for new
        // logs, overwriting the old data.  The value indicates how many
        // such files we will keep around at any point in time for later
        // use.  This is more efficient because the blocks are already
        // allocated and fdatasync does not need to update the inode after
        // each write.
        // Default: 0
        size_t recycle_log_file_num = 0;

        // manifest file is rolled over on reaching this limit.
        // The older manifest file be deleted.
        // The default value is 1GB so that the manifest file can grow, but not
        // reach the limit of storage capacity.
        uint64_t max_manifest_file_size = 1024 * 1024 * 1024;

        // Number of shards used for table cache.
        int table_cache_numshardbits = 6;

        // The following two fields affect when WALs will be archived and deleted.
        //
        // When both are zero, obsolete WALs will not be archived and will be deleted
        // immediately. Otherwise, obsolete WALs will be archived prior to deletion.
        //
        // When `WAL_size_limit_MB` is nonzero, archived WALs starting with the
        // earliest will be deleted until the total size of the archive falls below
        // this limit. All empty WALs will be deleted.
        //
        // When `WAL_ttl_seconds` is nonzero, archived WALs older than
        // `WAL_ttl_seconds` will be deleted.
        //
        // When only `WAL_ttl_seconds` is nonzero, the frequency at which archived
        // WALs are deleted is every `WAL_ttl_seconds / 2` seconds. When only
        // `WAL_size_limit_MB` is nonzero, the deletion frequency is every ten
        // minutes. When both are nonzero, the deletion frequency is the minimum of
        // those two values.
        uint64_t WAL_ttl_seconds = 0;
        uint64_t WAL_size_limit_MB = 0;

        // Number of bytes to preallocate (via fallocate) the manifest
        // files.  Default is 4mb, which is reasonable to reduce random IO
        // as well as prevent overallocation for mounts that preallocate
        // large amounts of data (such as xfs's allocsize option).
        size_t manifest_preallocation_size = 4 * 1024 * 1024;

        // Allow the OS to mmap file for reading sst tables.
        // Not recommended for 32-bit OS.
        // When the option is set to true and compression is disabled, the blocks
        // will not be copied and will be read directly from the mmap-ed memory
        // area, and the block will not be inserted into the block cache. However,
        // checksums will still be checked if ReadOptions.verify_checksums is set
        // to be true. It means a checksum check every time a block is read, more
        // than the setup where the option is set to false and the block cache is
        // used. The common use of the options is to run RocksDB on ramfs, where
        // checksum verification is usually not needed.
        // Default: false
        bool allow_mmap_reads = false;

        // Allow the OS to mmap file for writing.
        // DB::SyncWAL() only works if this is set to false.
        // Default: false
        bool allow_mmap_writes = false;

        // Enable direct I/O mode for read/write
        // they may or may not improve performance depending on the use case
        //
        // Files will be opened in "direct I/O" mode
        // which means that data r/w from the disk will not be cached or
        // buffered. The hardware buffer of the devices may however still
        // be used. Memory mapped files are not impacted by these parameters.

        // Use O_DIRECT for user and compaction reads.
        // Default: false
        bool use_direct_reads = false;

        // Use O_DIRECT for writes in background flush and compactions.
        // Default: false
        bool use_direct_io_for_flush_and_compaction = false;

        // If false, fallocate() calls are bypassed, which disables file
        // preallocation. The file space preallocation is used to increase the file
        // write/append performance. By default, RocksDB preallocates space for WAL,
        // SST, Manifest files, the extra space is truncated when the file is written.
        // Warning: if you're using btrfs, we would recommend setting
        // `allow_fallocate=false` to disable preallocation. As on btrfs, the extra
        // allocated space cannot be freed, which could be significant if you have
        // lots of files. More details about this limitation:
        // https://github.com/btrfs/btrfs-dev-docs/blob/471c5699336e043114d4bca02adcd57d9dab9c44/data-extent-reference-counts.md
        bool allow_fallocate = true;

        // Disable child process inherit open files. Default: true
        bool is_fd_close_on_exec = true;

        // if not zero, dump rocksdb.stats to LOG every stats_dump_period_sec
        //
        // Default: 600 (10 min)
        //
        // Dynamically changeable through SetDBOptions() API.
        unsigned int stats_dump_period_sec = 600;

        // if not zero, dump rocksdb.stats to RocksDB every stats_persist_period_sec
        // Default: 600
        unsigned int stats_persist_period_sec = 600;

        // If true, automatically persist stats to a hidden column family (column
        // family name: ___rocksdb_stats_history___) every
        // stats_persist_period_sec seconds; otherwise, write to an in-memory
        // struct. User can query through `GetStatsHistory` API.
        // If user attempts to create a column family with the same name on a DB
        // which have previously set persist_stats_to_disk to true, the column family
        // creation will fail, but the hidden column family will survive, as well as
        // the previously persisted statistics.
        // When peristing stats to disk, the stat name will be limited at 100 bytes.
        // Default: false
        bool persist_stats_to_disk = false;

        // if not zero, periodically take stats snapshots and store in memory, the
        // memory size for stats snapshots is capped at stats_history_buffer_size
        // Default: 1MB
        size_t stats_history_buffer_size = 1024 * 1024;

        // If set true, will hint the underlying file system that the file
        // access pattern is random, when a sst file is opened.
        // Default: true
        bool advise_random_on_open = true;

        // Amount of data to build up in memtables across all column
        // families before writing to disk.
        //
        // This is distinct from write_buffer_size, which enforces a limit
        // for a single memtable.
        //
        // This feature is disabled by default. Specify a non-zero value
        // to enable it.
        //
        // Default: 0 (disabled)
        size_t db_write_buffer_size = 0;

        // The memory usage of memtable will report to this object. The same object
        // can be passed into multiple DBs and it will track the sum of size of all
        // the DBs. If the total size of all live memtables of all the DBs exceeds
        // a limit, a flush will be triggered in the next DB to which the next write
        // is issued, as long as there is one or more column family not already
        // flushing.
        //
        // If the object is only passed to one DB, the behavior is the same as
        // db_write_buffer_size. When write_buffer_manager is set, the value set will
        // override db_write_buffer_size.
        //
        // This feature is disabled by default. Specify a non-zero value
        // to enable it.
        //
        // Default: null
        std::shared_ptr<WriteBufferManager> write_buffer_manager = nullptr;

        // If non-zero, we perform bigger reads when doing compaction. If you're
        // running RocksDB on spinning disks, you should set this to at least 2MB.
        // That way RocksDB's compaction is doing sequential instead of random reads.
        //
        // Default: 2MB
        //
        // Dynamically changeable through SetDBOptions() API.
        size_t compaction_readahead_size = 2 * 1024 * 1024;

        // This is the maximum buffer size that is used by WritableFileWriter.
        // With direct IO, we need to maintain an aligned buffer for writes.
        // We allow the buffer to grow until it's size hits the limit in buffered
        // IO and fix the buffer size when using direct IO to ensure alignment of
        // write requests if the logical sector size is unusual
        //
        // Default: 1024 * 1024 (1 MB)
        //
        // Dynamically changeable through SetDBOptions() API.
        size_t writable_file_max_buffer_size = 1024 * 1024;

        // Use adaptive mutex, which spins in the user space before resorting
        // to kernel. This could reduce context switch when the mutex is not
        // heavily contended. However, if the mutex is hot, we could end up
        // wasting spin time.
        // Default: false
        bool use_adaptive_mutex = false;

        // Create DBOptions with default values for all fields
        DBOptions();
        // Create DBOptions from Options
        explicit DBOptions(const Options &options);

        void Dump(Logger *log) const;

        // Allows OS to incrementally sync files to disk while they are being
        // written, asynchronously, in the background. This operation can be used
        // to smooth out write I/Os over time. Users shouldn't rely on it for
        // persistence guarantee.
        // Issue one request for every bytes_per_sync written. 0 turns it off.
        //
        // You may consider using rate_limiter to regulate write rate to device.
        // When rate limiter is enabled, it automatically enables bytes_per_sync
        // to 1MB.
        //
        // This option applies to table files
        //
        // Default: 0, turned off
        //
        // Note: DOES NOT apply to WAL files. See wal_bytes_per_sync instead
        // Dynamically changeable through SetDBOptions() API.
        uint64_t bytes_per_sync = 0;

        // Same as bytes_per_sync, but applies to WAL files
        // This does not gaurantee the WALs are synced in the order of creation. New
        // WAL can be synced while an older WAL doesn't. Therefore upon system crash,
        // this hole in the WAL data can create partial data loss.
        //
        // Default: 0, turned off
        //
        // Dynamically changeable through SetDBOptions() API.
        uint64_t wal_bytes_per_sync = 0;

        // When true, guarantees WAL files have at most `wal_bytes_per_sync`
        // bytes submitted for writeback at any given time, and SST files have at most
        // `bytes_per_sync` bytes pending writeback at any given time. This can be
        // used to handle cases where processing speed exceeds I/O speed during file
        // generation, which can lead to a huge sync when the file is finished, even
        // with `bytes_per_sync` / `wal_bytes_per_sync` properly configured.
        //
        //  - If `sync_file_range` is supported it achieves this by waiting for any
        //    prior `sync_file_range`s to finish before proceeding. In this way,
        //    processing (compression, etc.) can proceed uninhibited in the gap
        //    between `sync_file_range`s, and we block only when I/O falls behind.
        //  - Otherwise the `WritableFile::Sync` method is used. Note this mechanism
        //    always blocks, thus preventing the interleaving of I/O and processing.
        //
        // Note: Enabling this option does not provide any additional persistence
        // guarantees, as it may use `sync_file_range`, which does not write out
        // metadata.
        //
        // Default: false
        bool strict_bytes_per_sync = false;

        // A vector of EventListeners whose callback functions will be called
        // when specific RocksDB event happens.
        std::vector<std::shared_ptr<EventListener>> listeners;

        // If true, then the status of the threads involved in this DB will
        // be tracked and available via GetThreadList() API.
        //
        // Default: false
        bool enable_thread_tracking = false;

        // The limited write rate to DB if soft_pending_compaction_bytes_limit or
        // level0_slowdown_writes_trigger is triggered, or we are writing to the
        // last mem table allowed and we allow more than 3 mem tables. It is
        // calculated using size of user write requests before compression.
        // RocksDB may decide to slow down more if the compaction still
        // gets behind further.
        // If the value is 0, we will infer a value from `rater_limiter` value
        // if it is not empty, or 16MB if `rater_limiter` is empty. Note that
        // if users change the rate in `rate_limiter` after DB is opened,
        // `delayed_write_rate` won't be adjusted.
        //
        // Unit: byte per second.
        //
        // Default: 0
        //
        // Dynamically changeable through SetDBOptions() API.
        uint64_t delayed_write_rate = 0;

        // By default, a single write thread queue is maintained. The thread gets
        // to the head of the queue becomes write batch group leader and responsible
        // for writing to WAL and memtable for the batch group.
        //
        // If enable_pipelined_write is true, separate write thread queue is
        // maintained for WAL write and memtable write. A write thread first enter WAL
        // writer queue and then memtable writer queue. Pending thread on the WAL
        // writer queue thus only have to wait for previous writers to finish their
        // WAL writing but not the memtable writing. Enabling the feature may improve
        // write throughput and reduce latency of the prepare phase of two-phase
        // commit.
        //
        // Default: false
        bool enable_pipelined_write = false;

        // Setting unordered_write to true trades higher write throughput with
        // relaxing the immutability guarantee of snapshots. This violates the
        // repeatability one expects from ::Get from a snapshot, as well as
        // ::MultiGet and Iterator's consistent-point-in-time view property.
        // If the application cannot tolerate the relaxed guarantees, it can implement
        // its own mechanisms to work around that and yet benefit from the higher
        // throughput. Using TransactionDB with WRITE_PREPARED write policy and
        // two_write_queues=true is one way to achieve immutable snapshots despite
        // unordered_write.
        //
        // By default, i.e., when it is false, rocksdb does not advance the sequence
        // number for new snapshots unless all the writes with lower sequence numbers
        // are already finished. This provides the immutability that we expect from
        // snapshots. Moreover, since Iterator and MultiGet internally depend on
        // snapshots, the snapshot immutability results into Iterator and MultiGet
        // offering consistent-point-in-time view. If set to true, although
        // Read-Your-Own-Write property is still provided, the snapshot immutability
        // property is relaxed: the writes issued after the snapshot is obtained (with
        // larger sequence numbers) will be still not visible to the reads from that
        // snapshot, however, there still might be pending writes (with lower sequence
        // number) that will change the state visible to the snapshot after they are
        // landed to the memtable.
        //
        // Default: false
        bool unordered_write = false;

        // If true, allow multi-writers to update mem tables in parallel.
        // Only some memtable_factory-s support concurrent writes; currently it
        // is implemented only for SkipListFactory.  Concurrent memtable writes
        // are not compatible with inplace_update_support or filter_deletes.
        // It is strongly recommended to set enable_write_thread_adaptive_yield
        // if you are going to use this feature.
        //
        // Default: true
        bool allow_concurrent_memtable_write = true;

        // If true, threads synchronizing with the write batch group leader will
        // wait for up to write_thread_max_yield_usec before blocking on a mutex.
        // This can substantially improve throughput for concurrent workloads,
        // regardless of whether allow_concurrent_memtable_write is enabled.
        //
        // Default: true
        bool enable_write_thread_adaptive_yield = true;

        // The maximum limit of number of bytes that are written in a single batch
        // of WAL or memtable write. It is followed when the leader write size
        // is larger than 1/8 of this limit.
        //
        // Default: 1 MB
        uint64_t max_write_batch_group_size_bytes = 1 << 20;

        // The maximum number of microseconds that a write operation will use
        // a yielding spin loop to coordinate with other write threads before
        // blocking on a mutex.  (Assuming write_thread_slow_yield_usec is
        // set properly) increasing this value is likely to increase RocksDB
        // throughput at the expense of increased CPU usage.
        //
        // Default: 100
        uint64_t write_thread_max_yield_usec = 100;

        // The latency in microseconds after which a std::this_thread::yield
        // call (sched_yield on Linux) is considered to be a signal that
        // other processes or threads would like to use the current core.
        // Increasing this makes writer threads more likely to take CPU
        // by spinning, which will show up as an increase in the number of
        // involuntary context switches.
        //
        // Default: 3
        uint64_t write_thread_slow_yield_usec = 3;

        // If true, then DB::Open() will not update the statistics used to optimize
        // compaction decision by loading table properties from many files.
        // Turning off this feature will improve DBOpen time especially in
        // disk environment.
        //
        // Default: false
        bool skip_stats_update_on_db_open = false;

        // If true, then DB::Open() will not fetch and check sizes of all sst files.
        // This may significantly speed up startup if there are many sst files,
        // especially when using non-default Env with expensive GetFileSize().
        // We'll still check that all required sst files exist.
        // If paranoid_checks is false, this option is ignored, and sst files are
        // not checked at all.
        //
        // Default: false
        bool skip_checking_sst_file_sizes_on_db_open = false;

        // Recovery mode to control the consistency while replaying WAL
        // Default: kPointInTimeRecovery
        WALRecoveryMode wal_recovery_mode = WALRecoveryMode::kPointInTimeRecovery;

        // if set to false then recovery will fail when a prepared
        // transaction is encountered in the WAL
        bool allow_2pc = false;

        // A global cache for table-level rows.
        // Used to speed up Get() queries.
        // NOTE: does not work with DeleteRange() yet.
        // Default: nullptr (disabled)
        std::shared_ptr<RowCache> row_cache = nullptr;

        // A filter object supplied to be invoked while processing write-ahead-logs
        // (WALs) during recovery. The filter provides a way to inspect log
        // records, ignoring a particular record or skipping replay.
        // The filter is invoked at startup and is invoked from a single-thread
        // currently.
        WalFilter *wal_filter = nullptr;

        // DEPRECATED: This option might be removed in a future release.
        //
        // If true, then DB::Open, CreateColumnFamily, DropColumnFamily, and
        // SetOptions will fail if options file is not properly persisted.
        //
        // DEFAULT: true
        bool fail_if_options_file_error = true;

        // If true, then print malloc stats together with rocksdb.stats
        // when printing to LOG.
        // DEFAULT: false
        bool dump_malloc_stats = false;

        // By default RocksDB replay WAL logs and flush them on DB open, which may
        // create very small SST files. If this option is enabled, RocksDB will try
        // to avoid (but not guarantee not to) flush during recovery. Also, existing
        // WAL logs will be kept, so that if crash happened before flush, we still
        // have logs to recover from.
        //
        // DEFAULT: false
        bool avoid_flush_during_recovery = false;

        // By default RocksDB will flush all memtables on DB close if there are
        // unpersisted data (i.e. with WAL disabled) The flush can be skip to speedup
        // DB close. Unpersisted data WILL BE LOST.
        //
        // DEFAULT: false
        //
        // Dynamically changeable through SetDBOptions() API.
        bool avoid_flush_during_shutdown = false;

        // Set this option to true during creation of database if you want
        // to be able to ingest behind (call IngestExternalFile() skipping keys
        // that already exist, rather than overwriting matching keys).
        // Setting this option to true has the following effects:
        // 1) Disable some internal optimizations around SST file compression.
        // 2) Reserve the last level for ingested files only.
        // 3) Compaction will not include any file from the last level.
        // Note that only Universal Compaction supports allow_ingest_behind.
        // `num_levels` should be >= 3 if this option is turned on.
        //
        //
        // DEFAULT: false
        // Immutable.
        bool allow_ingest_behind = false;

        // If enabled it uses two queues for writes, one for the ones with
        // disable_memtable and one for the ones that also write to memtable. This
        // allows the memtable writes not to lag behind other writes. It can be used
        // to optimize MySQL 2PC in which only the commits, which are serial, write to
        // memtable.
        bool two_write_queues = false;

        // If true WAL is not flushed automatically after each write. Instead it
        // relies on manual invocation of FlushWAL to write the WAL buffer to its
        // file.
        bool manual_wal_flush = false;

        // If enabled WAL records will be compressed before they are written. Only
        // ZSTD (= kZSTD) is supported (until streaming support is adapted for other
        // compression types). Compressed WAL records will be read in supported
        // versions (>= RocksDB 7.4.0 for ZSTD) regardless of this setting when
        // the WAL is read.
        CompressionType wal_compression = kNoCompression;

        // Set to true to re-instate an old behavior of keeping complete, synced WAL
        // files open for write until they are collected for deletion by a
        // background thread. This should not be needed unless there is a
        // performance issue with file Close(), but setting it to true means that
        // Checkpoint might call LinkFile on a WAL still open for write, which might
        // be unsupported on some FileSystem implementations. As this is intended as
        // a temporary kill switch, it is already DEPRECATED.
        bool background_close_inactive_wals = false;

        // If true, RocksDB supports flushing multiple column families and committing
        // their results atomically to MANIFEST. Note that it is not
        // necessary to set atomic_flush to true if WAL is always enabled since WAL
        // allows the database to be restored to the last persistent state in WAL.
        // This option is useful when there are column families with writes NOT
        // protected by WAL.
        // For manual flush, application has to specify which column families to
        // flush atomically in DB::Flush.
        // For auto-triggered flush, RocksDB atomically flushes ALL column families.
        //
        // Currently, any WAL-enabled writes after atomic flush may be replayed
        // independently if the process crashes later and tries to recover.
        bool atomic_flush = false;

        // If true, working thread may avoid doing unnecessary and long-latency
        // operation (such as deleting obsolete files directly or deleting memtable)
        // and will instead schedule a background job to do it.
        // Use it if you're latency-sensitive.
        // If set to true, takes precedence over
        // ReadOptions::background_purge_on_iterator_cleanup.
        bool avoid_unnecessary_blocking_io = false;

        // The DB unique ID can be saved in the DB manifest (preferred, this option)
        // or an IDENTITY file (historical, deprecated), or both. If this option is
        // set to false (old behavior), then write_identity_file must be set to true.
        // The manifest is preferred because
        // 1. The IDENTITY file is not checksummed, so it is not as safe against
        //    corruption.
        // 2. The IDENTITY file may or may not be copied with the DB (e.g. not
        //    copied by BackupEngine), so is not reliable for the provenance of a DB.
        // This option might eventually be obsolete and removed as Identity files
        // are phased out.
        bool write_dbid_to_manifest = true;

        // It is expected that the Identity file will be obsoleted by recording
        // DB ID in the manifest (see write_dbid_to_manifest). Setting this to true
        // maintains the historical behavior of writing an Identity file, while
        // setting to false is expected to be the future default. This option might
        // eventually be obsolete and removed as Identity files are phased out.
        bool write_identity_file = true;

        // Historically, when prefix_extractor != nullptr, iterators have an
        // unfortunate default semantics of *possibly* only returning data
        // within the same prefix. To avoid "spooky action at a distance," iterator
        // bounds should come from the instantiation or seeking of the iterator,
        // not from a mutable column family option.
        //
        // When set to true, it is as if every iterator is created with
        // total_order_seek=true and only auto_prefix_mode=true and
        // prefix_same_as_start=true can take advantage of prefix seek optimizations.
        bool prefix_seek_opt_in_only = false;

        // The number of bytes to prefetch when reading the log. This is mostly useful
        // for reading a remotely located log, as it can save the number of
        // round-trips. If 0, then the prefetching is disabled.
        //
        // Default: 0
        size_t log_readahead_size = 0;

        // If user does NOT provide the checksum generator factory, the file checksum
        // will NOT be used. A new file checksum generator object will be created
        // when a SST file is created. Therefore, each created FileChecksumGenerator
        // will only be used from a single thread and so does not need to be
        // thread-safe.
        //
        // Default: nullptr
        std::shared_ptr<FileChecksumGenFactory> file_checksum_gen_factory = nullptr;

        // By default, RocksDB will attempt to detect any data losses or corruptions
        // in DB files and return an error to the user, either at DB::Open time or
        // later during DB operation. The exception to this policy is the WAL file,
        // whose recovery is controlled by the wal_recovery_mode option.
        //
        // Best-efforts recovery (this option set to true) signals a preference for
        // opening the DB to any point-in-time valid state for each column family,
        // including the empty/new state, versus the default of returning non-WAL
        // data losses to the user as errors. In terms of RocksDB user data, this
        // is like applying WALRecoveryMode::kPointInTimeRecovery to each column
        // family rather than just the WAL.
        //
        // The behavior changes in the presence of "AtomicGroup"s in the MANIFEST,
        // which is currently only the case when `atomic_flush == true`. In that
        // case, all pre-existing CFs must recover the atomic group in order for
        // that group to be applied in an all-or-nothing manner. This means that
        // unused/inactive CF(s) with invalid filesystem state can block recovery of
        // all other CFs at an atomic group.
        //
        // Best-efforts recovery (BER) is specifically designed to recover a DB with
        // files that are missing or truncated to some smaller size, such as the
        // result of an incomplete DB "physical" (FileSystem) copy. BER can also
        // detect when an SST file has been replaced with a different one of the
        // same size (assuming SST unique IDs are tracked in DB manifest).
        // BER is not yet designed to produce a usable DB from other corruptions to
        // DB files (which should generally be detectable by DB::VerifyChecksum()),
        // and BER does not yet attempt to recover any WAL files.
        //
        // For example, if an SST or blob file referenced by the MANIFEST is missing,
        // BER might be able to find a set of files corresponding to an old "point in
        // time" version of the column family, possibly from an older MANIFEST
        // file.
        // Besides complete "point in time" version, an incomplete version with
        // only a suffix of L0 files missing can also be recovered to if the
        // versioning history doesn't include an atomic flush.  From the users'
        // perspective, missing a suffix of L0 files means missing the
        // user's most recently written data. So the remaining available files still
        // presents a valid point in time view, although for some previous time. It's
        // not done for atomic flush because that guarantees a consistent view across
        // column families. We cannot guarantee that if recovering an incomplete
        // version.
        // Some other kinds of DB files (e.g. CURRENT, LOCK, IDENTITY) are
        // either ignored or replaced with BER, or quietly fixed regardless of BER
        // setting. BER does require at least one valid MANIFEST to recover to a
        // non-trivial DB state, unlike `ldb repair`.
        //
        // Default: false
        bool best_efforts_recovery = false;

        // It defines how many times DB::Resume() is called by a separate thread when
        // background retryable IO Error happens. When background retryable IO
        // Error happens, SetBGError is called to deal with the error. If the error
        // can be auto-recovered (e.g., retryable IO Error during Flush or WAL write),
        // then db resume is called in background to recover from the error. If this
        // value is 0 or negative, DB::Resume() will not be called automatically.
        //
        // Default: INT_MAX
        int max_bgerror_resume_count = INT_MAX;

        // If max_bgerror_resume_count is >= 2, db resume is called multiple times.
        // This option decides how long to wait to retry the next resume if the
        // previous resume fails and satisfy redo resume conditions.
        //
        // Default: 1000000 (microseconds).
        uint64_t bgerror_resume_retry_interval = 1000000;

        // It allows user to opt-in to get error messages containing corrupted
        // keys/values. Corrupt keys, values will be logged in the
        // messages/logs/status that will help users with the useful information
        // regarding affected data. By default value is set false to prevent users
        // data to be exposed in the logs/messages etc.
        //
        // Default: false
        bool allow_data_in_errors = false;

        // A string identifying the machine hosting the DB. This
        // will be written as a property in every SST file written by the DB (or
        // by offline writers such as SstFileWriter and RepairDB). It can be useful
        // for troubleshooting in memory corruption caused by a failing host when
        // writing a file, by tracing back to the writing host. These corruptions
        // may not be caught by the checksum since they happen before checksumming.
        // If left as default, the table writer will substitute it with the actual
        // hostname when writing the SST file. If set to an empty string, the
        // property will not be written to the SST file.
        //
        // Default: hostname
        std::string db_host_id = kHostnameForDbHostId;

        // Use this if your DB want to enable checksum handoff for specific file
        // types writes. Make sure that the File_system you use support the
        // crc32c checksum verification
        // Currently supported file tyes: kWALFile, kTableFile, kDescriptorFile.
        // NOTE: currently RocksDB only generates crc32c based checksum for the
        // handoff. If the storage layer has different checksum support, user
        // should enble this set as empty. Otherwise,it may cause unexpected
        // write failures.
        FileTypeSet checksum_handoff_file_types;

        // EXPERIMENTAL
        // CompactionService is a feature allows the user to run compactions on a
        // different host or process, which offloads the background load from the
        // primary host.
        // It's an experimental feature, the interface will be changed without
        // backward/forward compatibility support for now. Some known issues are still
        // under development.
        std::shared_ptr<CompactionService> compaction_service = nullptr;

        // It indicates, which lowest cache tier we want to
        // use for a certain DB. Currently we support volatile_tier and
        // non_volatile_tier. They are layered. By setting it to kVolatileTier, only
        // the block cache (current implemented volatile_tier) is used. So
        // cache entries will not spill to secondary cache (current
        // implemented non_volatile_tier), and block cache lookup misses will not
        // lookup in the secondary cache. When kNonVolatileBlockTier is used, we use
        // both block cache and secondary cache.
        //
        // Default: kNonVolatileBlockTier
        CacheTier lowest_used_cache_tier = CacheTier::kNonVolatileBlockTier;

        // DEPRECATED: This option might be removed in a future release.
        //
        // If set to false, when compaction or flush sees a SingleDelete followed by
        // a Delete for the same user key, compaction job will not fail.
        // Otherwise, compaction job will fail.
        // This is a temporary option to help existing use cases migrate, and
        // will be removed in a future release.
        // Warning: do not set to false unless you are trying to migrate existing
        // data in which the contract of single delete
        // (https://github.com/facebook/rocksdb/wiki/Single-Delete) is not enforced,
        // thus has Delete mixed with SingleDelete for the same user key. Violation
        // of the contract leads to undefined behaviors with high possibility of data
        // inconsistency, e.g. deleted old data become visible again, etc.
        bool enforce_single_del_contracts = true;

        // Implementing off-peak duration awareness in RocksDB. In this context,
        // "off-peak time" signifies periods characterized by significantly less read
        // and write activity compared to other times. By leveraging this knowledge,
        // we can prevent low-priority tasks, such as TTL-based compactions, from
        // competing with read and write operations during peak hours. Essentially, we
        // preprocess these tasks during the preceding off-peak period, just before
        // the next peak cycle begins. For example, if the TTL is configured for 25
        // days, we may compact the files during the off-peak hours of the 24th day.
        //
        // Time of the day in UTC, start_time-end_time inclusive.
        // Format - HH:mm-HH:mm (00:00-23:59)
        // If the start time > end time, it will be considered that the time period
        // spans to the next day (e.g., 23:30-04:00). To make an entire day off-peak,
        // use "0:00-23:59". To make an entire day have no offpeak period, leave
        // this field blank. Default: Empty string (no offpeak).
        std::string daily_offpeak_time_utc = "";

        // EXPERIMENTAL

        // When a RocksDB database is opened in follower mode, this option
        // is set by the user to request the frequency of the follower
        // attempting to refresh its view of the leader. RocksDB may choose to
        // trigger catch ups more frequently if it detects any changes in the
        // database state.
        // Default every 10s.
        uint64_t follower_refresh_catchup_period_ms = 10000;

        // For a given catch up attempt, this option specifies the number of times
        // to tail the MANIFEST and try to install a new, consistent  version before
        // giving up. Though it should be extremely rare, the catch up may fail if
        // the leader is mutating the LSM at a very high rate and the follower is
        // unable to get a consistent view.
        // Default to 10 attempts
        uint64_t follower_catchup_retry_count = 10;

        // Time to wait between consecutive catch up attempts
        // Default 100ms
        uint64_t follower_catchup_retry_wait_ms = 100;

        // When DB files other than SST, blob and WAL files are created, use this
        // filesystem temperature. (See also `wal_write_temperature` and various
        // `*_temperature` CF options.) When not `kUnknown`, this overrides any
        // temperature set by OptimizeForManifestWrite functions.
        Temperature metadata_write_temperature = Temperature::kUnknown;

        // Use this filesystem temperature when creating WAL files. When not
        // `kUnknown`, this overrides any temperature set by OptimizeForLogWrite
        // functions.
        Temperature wal_write_temperature = Temperature::kUnknown;
        // End EXPERIMENTAL
    };
}