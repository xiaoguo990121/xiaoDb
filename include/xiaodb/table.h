#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "xiaodb/cache.h"
#include "xiaodb/customizable.h"
#include "xiaodb/env.h"
#include "xiaodb/options.h"
#include "xiaodb/status.h"

namespace XIAODB_NAMESPACE
{
    // -- Block-based Table
    class Cache;
    class FilterPolicy;
    class FlushBlockPolicyFactory;
    class PersistentCache;
    class RandomAccessFile;
    struct TableReaderOptions;
    struct TableBuilderOptions;
    class TableBuilder;
    class TableFactory;
    class TableReader;
    class WritableFileWriter;
    struct ConfigOptions;
    struct EnvOptions;

    // Types of checksums to use for checking integrity of logical blocks within
    // files. All checksums currently use 32 bits of checking power (1 in 4B
    // chance of failing to detect random corruption). Traditionally, the actual
    // checking power can be far from ideal if the corruption is due to misplaced
    // data (e.g. physical blocks out of order in a file, or from another file),
    // which is fixed in format_version=6 (see below).
    enum ChecksumType : char
    {
        kNoChecksum = 0x0,
        kCRC32c = 0x1,
        kxxHash = 0x2,
        kxxHash64 = 0x3,
        kXXH3 = 0x4, // Supported since RocksDB 6.27
    };
}