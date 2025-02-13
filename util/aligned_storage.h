#pragma once

#include <cstddef>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    // 目的是为任意类型T提供一个对齐的内存区域。这个区域的大小与类型T相同，并且
    // 可以保证按照T或指定的对齐方式进行对齐。
    template <typename T, std::size_t Align = alignof(T)>
    struct aligned_storage
    {
        struct type
        {
            alignas(Align) unsigned char data[sizeof(T)];
        };
    };
}