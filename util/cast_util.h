#pragma once

#include <initializer_list>
#include <memory>
#include <type_traits>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
        // The helper function to assert the move from dynamic_cast<> to
        // static_cast<> is convert. This function is to deal with legacy code.
        // It is not recommended to add new code to issue class casting. The prefered
        // solution is to implement the functionality without a need of casting.
        template <class DestClass, class SrcClass> // 用于在进行类型转换时添加额外的检查
        inline DestClass *static_cast_with_check(SrcClass *x)
        {
                DestClass *ret = static_cast<DestClass *>(x);
#ifdef XIAODB_USE_RTTI
                assert(ret == dynamic_cast<DestClass *>(x));
#endif
                return ret;
        }

        template <class DestClass, class SrcClass>
        inline std::shared_ptr<DestClass> static_cast_with_check(
            std::shared_ptr<SrcClass> &&x)
        {
#if defined(ROCKSDB_USE_RTTI) && !defined(NDEBUG)
                auto orig_raw = x.get();
#endif
                auto ret = std::static_pointer_cast<DestClass>(std::move(x));
#if defined(ROCKSDB_USE_RTTI) && !defined(NDEBUG)
                assert(ret.get() == dynamic_cast<DestClass *>(orig_raw));
#endif
                return ret;
        }
}