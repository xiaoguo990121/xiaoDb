#pragma once

// 用于指示给定条件是否更有可能为true或false
#if defined(__GUNC__) && __GUNC__ >= 4
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif