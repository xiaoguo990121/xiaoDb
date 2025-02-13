#pragma once

#include <memory>
#include <string>

#include "xiaodb/customizable.h"
#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESAPCE
{

    class Slice;
    struct ConfigOptions;

    // A SliceTransform is a generic pluggable way of transforming one string
    // to another. Its primary use-case is in configuring XiaoDB prefix Bloom
    // filters, by setting prefix_extractor in ColumnFamilyOptions.
    //
    // Exceptions MUST NOT propagate out of overridden functions into XiaoDB,
    // because XiaoDB is not exception-safe. This could cause undefined behavior
    // including data loss, unreported corruption, deadlocks, and more.
    class SliceTransform : public Customizable
    {
    public:
        virtual ~SliceTransform() {}

        const char *Name() const override = 0;
        static const char *Type() { return "SliceTransform"; }

        static Status CreateFromString(const ConfigOptions &config_options,
                                       const std::string &id,
                                       std::shared_ptr<const SliceTransform> *result);

        std::string AsString() const;

        virtual Slice Transform(const Slice &key) const = 0;

        virtual bool InDomain(const Slice &key) const = 0;

        virtual bool InRange(const Slice &) const { return false; }

        virtual bool FullLengthEnabled(size_t) const { return false; }

        virtual bool SameResultWhenAppended(const Slice &) const
        {
            return false;
        }
    };

    const SliceTransform *NewFixedPrefixTransform(size_t prefix_len);

    const SliceTransform *NewCappedPrefixTransform(size_t cap_len);

    const SliceTransform *NewNoopTransform();
}