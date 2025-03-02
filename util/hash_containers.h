#pragma once

#include "xiaodb/xiaodb_namespace.h"

#ifdef USE_FOLLY

#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>

namespace ROCKSDB_NAMESPACE
{

    template <typename K, typename V>
    using UnorderedMap = folly::F14FastMap<K, V>;

    template <typename K, typename V, typename H>
    using UnorderedMapH = folly::F14FastMap<K, V, H>;

    template <typename K>
    using UnorderedSet = folly::F14FastSet<K>;

} // namespace ROCKSDB_NAMESPACE

#else

#include <unordered_map>
#include <unordered_set>

namespace XIAODB_NAMESPACE
{
    template <typename K, typename V>
    using UnorderedMap = std::unordered_map<K, V>;

    template <typename K, typename V, typename H>
    using UnorderedMapH = std::unordered_map<K, V, H>;

    template <typename K>
    using UnorderedSet = std::unordered_set<K>;
}
#endif