#pragma once

#include "xiaodb/env.h"

namespace XIAODB_NAMESPACE
{
    // TraceWriter allows exporting RocksDB traces to any system, one operation at
    // a time.
    class TraceWriter
    {
    public:
        virtual ~TraceWriter() = default;

        virtual Status Write(const Slice &data) = 0;
        virtual Status Close() = 0;
        virtual uint64_t GetFileSize() = 0;
    };

    // TraceReader allows reading RocksDB traces from any system, one operation at
    // a time. A RocksDB Replayer could depend on this to replay operations.
    class TraceReader
    {
    public:
        virtual ~TraceReader() = default;

        virtual Status Read(std::string *data) = 0;
        virtual Status Close() = 0;

        // Seek back to the trace header. Replayer can call this method to restart
        // replaying. Note this method may fail if the reader is already closed.
        virtual Status Reset() = 0;
    };

    // Factory methods to write/read traces to/from a file.
    // The implementations may not be thread-safe.
    Status NewFileTraceWriter(Env *env, const EnvOptions &env_options,
                              const std::string &trace_filename,
                              std::unique_ptr<TraceWriter> *trace_writer);
    Status NewFileTraceReader(Env *env, const EnvOptions &env_options,
                              const std::string &trace_filename,
                              std::unique_ptr<TraceReader> *trace_reader);
}