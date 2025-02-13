#pragma once

#include "util/random.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <thread>
#include <utility>

#include "port/likely.h"
#include "util/aligned_storage.h"
#include "util/thread_local.h"

#define STORAGE_DECL static thread_local

namespace XIAODB_NAMESPACE
{

    Random *Random::GetTLSInstance()
    {
        STORAGE_DECL Random *tls_instance;
        STORAGE_DECL aligned_storage<Random>::type tls_instance_bytes;

        auto rv = tls_instance;
        if (UNLIKELY(rv == nullptr))
        {
            size_t seed = std::hash<std::thread::id>()(std::this_thread::get_id());
            rv = new (&tls_instance_bytes) Random((uint32_t)seed);
            tls_instance = rv;
        }
        return rv;
    }

    std::string Random::HumanReadableString(int len)
    {
        std::string ret;
        ret.resize(len);
        for (int i = 0; i < len; ++i)
        {
            ret[i] = static_cast<char>('a' + Uniform(26));
        }
        return ret;
    }

    std::string Random::RandomString(int len)
    {
        std::string ret;
        ret.resize(len);
        for (int i = 0; i < len; i++)
        {
            ret[i] = static_cast<char>(' ' + Uniform(95));
        }
        return ret;
    }

    std::string Random::RandomBinaryString(int len)
    {
        std::string ret;
        ret.resize(len);
        for (int i = 0; i < len; i++)
        {
            ret[i] = static_cast<char>(Uniform(CHAR_MAX));
        }
        return ret;
    }

}
