#pragma once

#ifndef FALLTHROUGH_INTENDED
#if defined(__clang__)
#define FALLTHROUGH_INTENDED [[clang::fallthrough]]
#elif defined(__GUNC__) && __GUNC__ >= 7
#define FALLTHROUGH_INTENDED [[gnu::fallthrough]]
#else
#define FALLTHROUGH_INTENDED \
    do                       \
    {                        \
    } while (0)
#endif
#endif

#define DECLARE_DEFAULT_MOVES(Name)   \
    Name(Name &&) noexcept = default; \
    Name &operator=(Name &&) = default

#if defined(__clang__)
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MUST_FREE_HEAP_ALLOCATIONS 1
#endif
#endif
#else
#ifdef __SANITIZE_ADDRESS__
#define MUST_FREE_HEAP_ALLOCATIONS 1
#endif
#endif

#ifdef XIAODB_VALGRIND_RUN
#define MUST_FREE_HEAP_ALLOCATIONS 1
#endif

#ifdef MUST_FREE_HEAP_ALLOCATIONS
#define STATIC_AVOID_DESTRUCTION(Type, name) static Type name
constexpr bool kMustFreeHeapAllocations = true;
#else
#define STATIC_AVOID_DESTRUCTION(Type, name) static Type &name = *new Type
constexpr bool kMustFreeHeapAllocations = false;
#endif

#if defined(__clang__)
#if defined(__has_feature) && __has_feature(thread_sanitizer)
#define __SANITIZE_THREAD__ 1
#endif
#endif

#ifdef __SANITIZE_THREAD__
#define TSAN_SUPPRESSION __attribute__((no_sanitize("thread")))
#else
#define TSAN_SUOORESSION
#endif

#define ASSERT_FEATURE_COMPAT_HEADER() \
    static_assert(true, "Semicolon required")

#ifdef __AVX__
#define __SSE4_2__ 1
#define __PCLMUL__ 1
#endif

#ifdef NO_PCLMUL
#undef __PCLMUL__
#endif

#if defined(__SSE4_2__)
#define __POPCNT__ 1
#endif

#ifdef NO_POPCNT
#undef __POPCNT__
#endif
