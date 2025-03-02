#pragma once

#include "xiaodb/advanced_cache.h"
#include "xiaodb/table.h"

namespace XIAODB_NAMESPACE
{
    class Footer;

    // Release the cached entry and decrement its ref count.
    void ForceReleaseCachedEntry(void *arg, void *h);

    inline MemoryAllocator *GetMemoryAllocator(
        const BlockBasedTableOptions &table_options)
    {
        return table_options.block_cache.get()
                   ? table_options.block_cache->memory_allocator()
                   : nullptr;
    }

    // Assumes block has a trailer past `data + block_size` as in format.h.
    // `file_name` provided for generating diagostic message in returned status.
    // `offset` might be required for proper verification (also used for message).
    //
    // Returns Status::OK() on checksum match, or Status::Corruption() on checksum
    // mismatch.
    Status VerifyBlockCheckSum(const Footer &footer, const char *data,
                               size_t block_size, const std::string &file_name,
                               uint64_t offset);
}