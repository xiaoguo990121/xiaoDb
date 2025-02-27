#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "db/blob/blob_constants.h"
#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{

    class JSONWriter;
    class Slice;
    class Status;

    class BlobFileGarbage
    {
    public:
        BlobFileGarbage() = default;

        BlobFileGarbage(uint64_t blob_file_number, uint64_t garbage_blob_count,
                        uint64_t garbage_blob_bytes)
            : blob_file_number_(blob_file_number),
              garbage_blob_count_(garbage_blob_count),
              garbage_blob_bytes_(garbage_blob_bytes) {}

        uint64_t GetBlobFileNumber() const { return blob_file_number_; }
        uint64_t GetGarbageBlobCount() const { return garbage_blob_count_; }
        uint64_t GetGarbageBlobBytes() const { return garbage_blob_bytes_; }

        void EncodeTo(std::string *output) const;
        Status DecodeFrom(Slice *input);

        std::string DebugString() const;
        std::string DebugJSON() const;

    private:
        enum CustomFieldTags : uint32_t;

        uint64_t blob_file_number_ = kInvalidBlobFileNumber;
        uint64_t garbage_blob_count_ = 0;
        uint64_t garbage_blob_bytes_ = 0;
    };

    bool operator==(const BlobFileGarbage &lhs, const BlobFileGarbage &rhs);
    bool operator!=(const BlobFileGarbage &lhs, const BlobFileGarbage &rhs);

    std::ostream &operator<<(std::ostream &os,
                             const BlobFileGarbage &blob_file_garbage);
    JSONWriter &operator<<(JSONWriter &jw,
                           const BlobFileGarbage &blob_file_garbage);
}