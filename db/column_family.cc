#include "db/column_family.h"

#include <algorithm>
#include <cinttypes>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "db/blob/blob_file_cache.h"
#include "db/blob/blob_source.h"
#include "util/cast_util.h"

namespace XIAODB_NAMESPACE
{
    uint32_t ColumnFamilyHandleImpl::GetID() const { return cfd()->GetID(); }

    uint32_t GetColumnFamilyID(ColumnFamilyHandle *column_family)
    {
        uint32_t column_family_id = 0;
        if (column_family != nullptr)
        {
            auto cfh = static_cast_with_check<ColumnFamilyHandleImpl>(column_family);
            column_family_id = cfh->GetID();
        }
        return column_family_id;
    }
}
