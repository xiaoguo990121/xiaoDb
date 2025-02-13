#pragma once

#ifdef OS_WIN
#include "port/win/port_win.h"
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

#include <cstdint>
#include <utility>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    // An RAII wrapper for mmaped memory
    class MemMapping
    {
    public:
        // 编译时检测当前平台是否支持大页内存
        static constexpr bool kHugePageSupported =
#if defined(MAP_HUGETLB) || defined(FILE_MAP_LARGE_PAGES)
            true;
#else
            false;
#endif
        // Allocate memory requesting to be backed by huge pages
        static MemMapping AllocateHuge(size_t length);

        // Allocate memory that is only lazily mapped to resident memory and
        // guaranteed to be zero-initialized. Note that some platforms like
        // Linux allow memory over-commit, where only the used portion of memory
        // matters, while other platforms require enough swap space (page file) to
        // back the full mapping.
        static MemMapping AllocateLazyZeroed(size_t length);

        MemMapping(const MemMapping &) = delete;
        MemMapping &operator=(const MemMapping &) = delete;

        MemMapping(MemMapping &&) noexcept;
        MemMapping &operator=(MemMapping &&) noexcept;

        ~MemMapping();

        inline void *Get() const { return addr_; }
        inline size_t Length() const { return length_; }

    private:
        MemMapping()
        {
        }

        // The mapped memory, or nullptr on failure / not supported
        void *addr_ = nullptr;
        // The known usable number of bytes starting at that address
        size_t length_ = 0;

#ifdef OS_WIN
        HANDLE page_file_handle_ = NULL;
#endif

        static MemMapping AllocateAnonymous(size_t length, bool huge);
    };

    /**
     * @brief Simple MemMapping wrapper that presents the memory as an array of T.
     * TypedMemMapping<uint64_t> arr = MemMapping::AllocateLazyZeroed(num_bytes);
     *
     * @tparam T
     */
    template <typename T>
    class TypedMemMapping : public MemMapping
    {
    public:
        TypedMemMapping(MemMapping &&v) noexcept
            : MemMapping(std::move(v)) {}
        TypedMemMapping &operator=(MemMapping &&v) noexcept
        {
            MemMapping &base = *this;
            base = std::move(v);
        }

        inline T *Get() const { return static_cast<T *>(MemMapping::Get()); }
        inline size_t Count() const { return MemMapping::Length() / sizeof(T); }

        inline T &operator[](size_t index) const { return Get()[index]; }
    };
}