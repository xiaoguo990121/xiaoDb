#include "cache/cache_entry_roles.h"

#include <mutex>

#include "port/lang.h"

namespace XIAODB_NAMESPACE
{

    std::array<std::string, kNumCacheEntryRoles> kCacheEntryRoleToCamelString{{
        "DataBlock",
        "FilterBlock",
        "FilterMetaBlock",
        "DeprecatedFilterBlock",
        "IndexBlock",
        "OtherBlock",
        "WriteBuffer",
        "CompressionDictionaryBuildingBuffer",
        "FilterConstruction",
        "BlockBasedTableReader",
        "FileMetadata",
        "BlobValue",
        "BlobCache",
        "Misc",
    }};

    std::array<std::string, kNumCacheEntryRoles> kCacheEntryRoleToHyphenString{{
        "data-block",
        "filter-block",
        "filter-meta-block",
        "deprecated-filter-block",
        "index-block",
        "other-block",
        "write-buffer",
        "compression-dictionary-building-buffer",
        "filter-construction",
        "block-based-table-reader",
        "file-metadata",
        "blob-value",
        "blob-cache",
        "misc",
    }};

    const std::string &GetCacheEntryRoleName(CacheEntryRole role)
    {
        return kCacheEntryRoleToHyphenString[static_cast<size_t>(role)];
    }
}