#pragma once

#include <string>
#include <unordered_map>

#include "xiaodb/xiaodb_namespace.h"

#define XIAODB_MAJOR 0
#define XIAODB_MINOR 0
#define XIAODB_PATCH 1

namespace XIAODB_NAMESPACE
{

    const std::unordered_map<std::string, std::string> &GetRocksBuildProperties();

    std::string GetXiaosVersionAsString(bool with_patch = true);

    std::string GetXiaosBuildInfoAsString(const std::string &program,
                                          bool verbose = false);
}