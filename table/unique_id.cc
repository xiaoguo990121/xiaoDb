#include <cstdint>

#include "table/unique_id_impl.h"
#include "util/coding_lean.h"
#include "util/hash.h"
#include "util/string_util.h"

namespace XIAODB_NAMESPACE
{

    Status GetSstInternalUniqueId(const std::string &db_id,
                                  const std::string &db_session_id,
                                  uint64_t file_number, UniqueIdPtr out,
                                  bool force)
    {
        if (!force)
        {
            if (db_id.empty())
            {
                return Status::NotSupported("Missing db_id");
            }
            if (file_number == 0)
            {
                return Status::NotSupported("Missing or bad file number");
            }
            if (db_session_id.empty())
            {
                return Status::NotSupported("Missing db_session_id");
            }
        }
        uint64_t session_upper = 0;
        uint64_t session_lower = 0;
        {
            Status s = DecodeSessionId(db_session_id, &session_upper, &session_lower);
            if (!s.ok())
            {
                if (!force)
                {
                    return s;
                }
                else
                {
                    // A reasonable fallback in case malformed
                    Hash2x64(db_session_id.data(), db_session_id.size(), &session_upper,
                             &session_lower);
                    if (session_lower == 0)
                    {
                        session_lower = session_upper | 1;
                    }
                }
            }
        }

        // Exactly preserve session lower to ensure that session ids generated
        // during the same process lifetime are guaranteed unique.
        // DBImpl also guarantees (in recent versions) that this is not zero,
        // so that we can guarantee unique ID is never all zeros. (Can't assert
        // that here because of testing and old versions.)
        // We put this first in anticipation of matching a small-ish set of cache
        // key prefixes to cover entries relevant to any DB.
        out.ptr[0] = session_lower;

        // Hash the session upper (~39 bits entropy) and DB id (120+ bits entropy)
        // for very high global uniqueness entropy.
        // (It is possible that many DBs descended from one common DB id are copied
        // around and proliferate, in which case session id is critical, but it is
        // more common for different DBs to have different DB ids.)
        uint64_t db_a, db_b;
        Hash2x64(db_id.data(), db_id.size(), session_upper, &db_a, &db_b);

        // Xor in file number for guaranteed uniqueness by file number for a given
        // session and DB id. (Xor slightly better than + here. See
        // https://github.com/pdillinger/unique_id )
        out.ptr[1] = db_a ^ file_number;

        // Extra (optional) global uniqueness
        if (out.extended)
        {
            out.ptr[2] = db_b;
        }

        return Status::OK();
    }
}