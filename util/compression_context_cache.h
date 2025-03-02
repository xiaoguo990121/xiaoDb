#pragma once

#include <stdint.h>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    class ZSTDUncompressCacheData;

    class CompressionContextCache
    {
    public:
        // Singleton
        static CompressionContextCache *Instance();
        static void InitSingleton();
        CompressionContextCache(const CompressionContextCache &) = delete;
        CompressionContextCache &operator=(const CompressionContextCache &) = delete;

        ZSTDUncompressCacheData GetCachedZSTDUncompressData();
        void ReturnCachedZSTDUncompressData(int64_t idx);

    private:
        // Singleton
        CompressionContextCache();
        ~CompressionContextCache();

        class Rep;
        Rep *rep_;
    };
}