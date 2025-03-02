#pragma once

#include <cstdint>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    // Represents the types of blocks used in the block based table format.

    enum class BlockType : uint8_t
    {
        kData,
        kFilter,               // for second level partitioned filters and full filters
        kFilterPartitionIndex, // for top-level index of filter partitions
        kProperties,
        kCompressionDictionary,
        kRangeDeletion,
        kHashIndexPrefixes,
        kHashIndexMetadata,
        kMetaIndex,
        kIndex,
        // Note: keep kInvalid the last value when adding new enum values.
        kInvalid
    };
}