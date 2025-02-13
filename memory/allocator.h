#pragma once

#include <cerrno>
#include <cstddef>

#include "xiaodb/write_buffer_manager.h"

namespace XIAODB_NAMESPACE
{
    class Logger;

    class Allocator
    {
    public:
        virtual ~Allocator() {}

        virtual char *Allocate(size_t bytes) = 0;
        virtual char *AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                                      Logger *logger = nullptr) = 0;

        virtual size_t BlockSize() const = 0;
    };

    class AllocTracker
    {
    public:
        explicit AllocTracker(WriteBufferManager *write_buffer_manager);

        AllocTracker(const AllocTracker &) = delete;
        void operator=(const AllocTracker &) = delete;

        ~AllocTracker();
        void Allocate(size_t bytes);

        void DoneAllocating();

        void FreeMem();

        bool is_freed() const { return write_buffer_manager_ == nullptr || freed_; }

    private:
        WriteBufferManager *write_buffer_manager_;
        std::atomic<size_t> bytes_allocated_;
        bool done_allocating_;
        bool freed_;
    };
}