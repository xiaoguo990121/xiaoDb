#pragma once

#include "xiaodb/xiaodb_namespace.h"
#include "xiaodb/type.h"

namespace XIAODB_NAMESPACE
{
    enum CompressionType : unsigned char
    {
        kNoCompression = 0x0,
        kSnappyCompression = 0x1,
        kZlibCompression = 0x2,
        kBZip2Compression = 0x3,
        kLZ4Compression = 0x4,
        kXpressCompression = 0x6,
        kZSTD = 0x7,

        kDisableCompressionOption = 0xff,
    };

    struct CompressionOptions
    {
        static constexpr int kDefaultCompressionLevel = 32767;

        int window_bits = -14;

        int level = kDefaultCompressionLevel;

        int strategy = 0;

        uint32_t max_dict_bytes = 0;

        uint32_t zstd_max_train_bytes = 0;

        uint32_t parallel_threads = 1;

        bool enabled = false;

        uint64_t max_dict_buffer_bytes = 0;

        bool use_zstd_dict_trainer = true;

        int max_compressed_bytes_per_kb = 1024 * 7 / 8;

        bool checksum = false;

        void SetMinRatio(double min_ratio)
        {
            max_compressed_bytes_per_kb = static_cast<int>(1024.0 / min_ratio + 0.5);
        }
    };
}