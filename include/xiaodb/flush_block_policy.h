#pragma once

#include <string>

#include "xiaodb/customizable.h"
#include "xiaodb/table.h"

namespace XIAODB_NAMESPACE
{

    class Slice;
    class BlockBuilder;
    class ConfigOptions;
    class Options;

    // FlushBlockPolicy provides a configurable way to determine when to flush a
    // block in the block based tables.
    //
    // Exceptions MUST NOT propagate out of overridden functions into RocksDB,
    // because RocksDB is not exception-safe. This could cause undefined behavior
    // including data loss, unreported corruption, deadlocks, and more.
    class FlushBlockPolicy
    {
    public:
        // Keep track of the key/value sequences and return the boolean value to
        // determine if table builder should flush current data block.
        virtual bool Update(const Slice &key, const Slice &value) = 0;

        virtual ~FlushBlockPolicy() {}
    };

    class FlushBlockPolicyFactory : public Customizable
    {
    public:
        static const char *Type() { return "FlushBlockPolicyFactory"; }

        // Creates a FlushBlockPolicyFactory based on the input value.
        // By default, this method can create EveryKey or BySize PolicyFactory,
        // which take now config_options.
        static Status CreateFromString(
            const ConfigOptions &config_options, const std::string &value,
            std::shared_ptr<FlushBlockPolicyFactory> *result);

        // Return a new block flush policy that flushes data blocks by data size.
        // FlushBlockPolicy may need to access the metadata of the data block
        // builder to determine when to flush the blocks.
        //
        // Callers must delete the result after any database that is using the
        // result has been closed.
        virtual FlushBlockPolicy *NewFlushBlockPolicy(
            const BlockBasedTableOptions &table_options,
            const BlockBuilder &data_block_builder) const = 0;

        virtual ~FlushBlockPolicyFactory() {}
    };

    class FlushBlockBySizePolicyFactory : public FlushBlockPolicyFactory
    {
    public:
        FlushBlockBySizePolicyFactory();

        static const char *kClassName() { return "FlushBlockBySizePolicyFactory"; }
        const char *Name() const override { return kClassName(); }

        FlushBlockPolicy *NewFlushBlockPolicy(
            const BlockBasedTableOptions &table_options,
            const BlockBuilder &data_block_builder) const override;

        static FlushBlockPolicy *NewFlushBlockPolicy(
            const uint64_t size, const int deviation,
            const BlockBuilder &data_block_builder);
    };
}