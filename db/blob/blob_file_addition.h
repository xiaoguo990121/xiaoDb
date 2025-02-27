#pragma once

#include <cassert>
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

    class BlobFileAddition
    {
    public:
        BlobFileAddition() = default;

        BlobFileAddition(uint64_t blob_file_number, uint64_t total_blob_count,
                         uint64_t total_blob_bytes, std::string checksum_method,
                         std::string checksum_value)
            : blob_file_number_(blob_file_number),
              total_blob_count_(total_blob_count),
              total_blob_bytes_(total_blob_bytes),
              checksum_method_(std::move(checksum_method)),
              checksum_value_(std::move(checksum_value))
        {
            assert(checksum_method_.empty() == checksum_value_.empty());
        }

        uint64_t GetBlobFileNumber() const { return blob_file_number_; }
        uint64_t GetTotalBlobCount() const { return total_blob_count_; }
        uint64_t GetTotalBlobBytes() const { return total_blob_bytes_; }
        const std::string &GetChecksumMethod() const { return checksum_method_; }
        const std::string &GetChecksumValue() const { return checksum_value_; }

        void Encode(std::string *output) const;
        Status DecodeFrom(Slice *input);

        std::string DebugString() const;
        std::string DebugJson() const;

    private:
        enum CustomFieldTags : uint32_t;

        uint64_t blob_file_number_ = kInvalidBlobFileNumber;
        uint64_t total_blob_count_ = 0;
        uint64_t total_blob_bytes_ = 0;
        std::string checksum_method_;
        std::string checksum_value_;
    };

    bool operator==(const BlobFileAddition &lhs, const BlobFileAddition &rhs);
    bool operator!=(const BlobFileAddition &lhs, const BlobFileAddition &rhs);

    std::ostream &operator<<(std::ostream &os,
                             const BlobFileAddition &blob_file_addition);
    JSONWriter &operator<<(JSONWriter &jw,
                           const BlobFileAddition &blob_file_addition);
}