#pragma once

#include <array>
#include <cstdint>

#include "xiaodb/cache.h"

namespace XIAODB_NAMESPACE
{

    extern std::array<std::string, kNumCacheEntryRoles>
        kCacheEntryRoleToCamelString;
    extern std::array<std::string, kNumCacheEntryRoles>
        kCacheEntryRoleToHyphenString;
}