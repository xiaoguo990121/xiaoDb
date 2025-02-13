#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <intrin.h>
#include <malloc.h>
#include <process.h>
#include <stdint.h>
#include <string.h>

#include <cassert>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <string>
#include <thread>

#include "port/win/win_thread.h"
#include "xiaodb/port_defs.h"

#undef min
#undef max
#undef DeleteFile
#undef GetCurrentTime

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef _SSIZE_T_DEFINED
using ssize_t = SSIZE_T;
#endif

#ifndef XIAODB_PRIszt
#define XIAODB_PRIszt "Iu"
#endif

#ifdef _MSC_VER
#define __attribute__(A)

#endif

namespace XIAODB_NAMESPACE
{

#define PREFETCH(addr, rw, locality)

    extern const bool kDefaultToAdaptiveMutex;

    namespace port
    {
        constexpr bool kLittleEndian = true;
#undef PLATFORM_IS_LITTLE_ENDIAN

        class CondVar;

        class Mutex
        {
        public:
            static const char *kName() { return "std::mutex"; }

            explicit Mutex(bool IGNORED_adaptive = kDefaultToAdaptiveMutex)
#ifndef NDEBUG
                : locked_(false)
#endif
            {
                (void)IGNORED_adaptive;
            }

            ~Mutex();

            void Lock()
            {
                mutex_.lock();
#ifndef NDEBUG
                locked_ = true;
#endif
            }

            void Unlock()
            {
#ifndef NDEBUG
                locked_ = false;
#endif
                mutex_.unlock();
            }

            bool TryLock()
            {
                bool ret = mutex_.try_lock();
#ifndef NDEBUG
                if (ret)
                {
                    locked_ = true;
                }
#endif
                return ret;
            }

            void AssertHeld() const
            {
#ifndef NDEBUG
                assert(locked_);
#endif
            }

            inline void lock() { Lock(); }
            inline void unlock() { Unlock(); }
            inline bool try_lock() { return TryLock(); }

            Mutex(const Mutex &) = delete;
            void operator=(const Mutex &) = delete;

        private:
            friend class CondVar;

            std::mutex &getLock() { return mutex_; }

            std::mutex mutex_;
#ifndef NDEBUG
            bool locked_;
#endif
        };

        class RWMutex
        {
        public:
            RWMutex() { InitializeSRWLock(&srwLock_); }

            RWMutex(const RWMutex &) = delete;
            void operator=(const RWMutex &) = delete;

            void ReadLock() { AcquireSRWLockShared(&srwLock_); }

            void WriteLock() { AcquireSRWLockExclusive(&srwLock_); }

            void ReadUnlock() { ReleaseSRWLockShared(&srwLock_); }

            void WriteUnlock() { ReleaseSRWLockExclusive(&srwLock_); }

            void AssertHeld() const {}

        private:
            SRWLOCK srwLock_;
        };

        class CondVar
        {
        public:
            explicit CondVar(Mutex *mu) : mu_(mu) {}

            ~CondVar();

            Mutex *GetMutex() const { return mu_; }

            void Wait();
            bool TimeWait(uint64_t expireation_time);
            void Signal();
            void SignalAll();

            CondVar(const CondVar &) = delete;
            CondVar &operator=(const CondVar &) = delete;

            CondVar(CondVar &&) = delete;
            CondVar &operator=(CondVar &&) = delete;

        private:
            std::condition_variable cv_;
            Mutex *mu_;
        };

#ifdef _POSIX_THREADS
        using Thread = std::thread;
#else
        using Thread = WindowsThread;
#endif

        struct OnceType
        {
            struct Init
            {
            };

            OnceType() {}
            OnceType(const Init &) {}
            OnceType(const OnceType &) = delete;
            OnceType &operator=(const OnceType &) = delete;

            std::once_flag flag_;
        };

#define XIAODB_ONCE_INIT port::OnceType::Init()
        void InitOnce(OnceType *once, void (*initializer)());

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64U
#endif

#ifdef XIAODB_JEMALLOC
        void *jemalloc_aligned_alloc(size_t size, size_t alignment) noexcept;
        void jemalloc_aligned_free(void *p) noexcept;
#endif

        inline void *cacheline_aligned_alloc(size_t size)
        {
#ifdef XIAODB_JEMALLC
            return jemalloc_aligned_alloc(size, CACHE_LINE_SIZE);
#else
            return _aligned_malloc(size, CACHE_LINE_SIZE);
#endif
        }

        inline void cacheline_aligned_free(void *memblock)
        {
#ifdef XIAODB_JEMALLOC
            jemalloc_aligned_free(memblock);
#else
            _aligned_free(memblock);
#endif
        }

        extern const size_t kPageSize;

#define ALIGN_AS(n) alignas(n)

        static inline void AsmVolatilePause()
        {
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64) || defined(_M_ARM)
            YieldProcessor();
#endif
        }

        int PhysicalCoreID();

        using pthread_key_t = DWORD;

        inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
        {
            (void)destructor;

            pthread_key_t k = TlsAlloc();
            if (TLS_OUT_OF_INDEXES == k)
            {
                return ENOMEM;
            }

            *key = k;
            return 0;
        }

        inline int pthread_key_delete(pthread_key_t key)
        {
            if (TlsFree(key))
            {
                return EINVAL;
            }
            return 0;
        }

        inline int pthread_setspecific(pthread_key_t key, const void *value)
        {
            if (!TlsSetValue(key, const_cast<void *>(value)))
            {
                return ENOMEM;
            }
            return 0;
        }

        inline void *pthread_getspecific(pthread_key_t key)
        {
            void *result = TlsGetValue(key);
            if (!result)
            {
                if (GetLastError() != ERROR_SUCCESS)
                {
                    errno = EINVAL;
                }
                else
                {
                    errno = NOERROR;
                }
            }
            return result;
        }

        int truncate(const char *path, int64_t length);
        int Truncate(std::string path, int64_t length);
        void Crash(const std::string &srcfile, int srcline);
        int GetMaxOpenFiles();
        std::string utf16_to_utf8(const std::wstring &utf16);
        std::wstring utf8_to_utf16(const std::string &utf8);

        using ThreadId = int;

        void SetCpuPriority(ThreadId id, CpuPriority priority);

        int64_t GetProcessID();

        bool GenerateRfcUuid(std::string *output);

    }

#ifdef XIAODB_WINDOWS_UTF8_FILENAMES

#define RX_FILESTRING std::wstring

#define RX_FILESTRING std::wstring
#define RX_FN(a) ROCKSDB_NAMESPACE::port::utf8_to_utf16(a)
#define FN_TO_RX(a) ROCKSDB_NAMESPACE::port::utf16_to_utf8(a)
#define RX_FNCMP(a, b) ::wcscmp(a, RX_FN(b).c_str())
#define RX_FNLEN(a) ::wcslen(a)

#define RX_DeleteFile DeleteFileW
#define RX_CreateFile CreateFileW
#define RX_CreateFileMapping CreateFileMappingW
#define RX_GetFileAttributesEx GetFileAttributesExW
#define RX_FindFirstFileEx FindFirstFileExW
#define RX_FindNextFile FindNextFileW
#define RX_WIN32_FIND_DATA WIN32_FIND_DATAW
#define RX_CreateDirectory CreateDirectoryW
#define RX_RemoveDirectory RemoveDirectoryW
#define RX_GetFileAttributesEx GetFileAttributesExW
#define RX_MoveFileEx MoveFileExW
#define RX_CreateHardLink CreateHardLinkW
#define RX_PathIsRelative PathIsRelativeW
#define RX_GetCurrentDirectory GetCurrentDirectoryW
#define RX_GetDiskFreeSpaceEx GetDiskFreeSpaceExW
#define RX_PathIsDirectory PathIsDirectoryW

#else

#define RX_FILESTRING std::string
#define RX_FN(a) a
#define FN_TO_RX(a) a
#define RX_FNCMP(a, b) strcmp(a, b)
#define RX_FNLEN(a) strlen(a)

#define RX_DeleteFile DeleteFileA
#define RX_CreateFile CreateFileA
#define RX_CreateFileMapping CreateFileMappingA
#define RX_GetFileAttributesEx GetFileAttributesExA
#define RX_FindFirstFileEx FindFirstFileExA
#define RX_CreateDirectory CreateDirectoryA
#define RX_FindNextFile FindNextFileA
#define RX_WIN32_FIND_DATA WIN32_FIND_DATAA
#define RX_CreateDirectory CreateDirectoryA
#define RX_RemoveDirectory RemoveDirectoryA
#define RX_GetFileAttributesEx GetFileAttributesExA
#define RX_MoveFileEx MoveFileExA
#define RX_CreateHardLink CreateHardLinkA
#define RX_PathIsRelative PathIsRelativeA
#define RX_GetCurrentDirectory GetCurrentDirectoryA
#define RX_GetDiskFreeSpaceEx GetDiskFreeSpaceExA
#define RX_PathIsDirectory PathIsDirectoryA

#endif

    using port::pthread_getspecific;
    using port::pthread_key_create;
    using port::pthread_key_delete;
    using port::pthread_key_t;
    using port::pthread_setspecific;
    using port::truncate;

}