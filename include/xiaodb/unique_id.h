#pragma once

#include "xiaodb/table_properties.h"

namespace XIAODB_NAMESPACE
{
    Status GetUniqueIdFromTableProperties(const TableProperties &props,
                                          std::string *out_id);

    Status GetExtendedUniqueIdFromTableProperties(const TableProperties &props,
                                                  std::string *out_id);

    std::string UniqueIdToHumanString(const std::string &id);
}