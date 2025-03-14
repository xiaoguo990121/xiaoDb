#include "db/write_thread.h"

#include <chrono>
#include <thread>

#include "port/port.h"
#include "util/random.h"

namespace XIAODB_NAMESPACE
{

    WriteThread::WriteThread(const ImmutableDBOptions &dp_options)
        : max_yield_usec_(db_options.enable_)
    {
    }
}