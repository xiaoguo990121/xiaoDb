#include "options/options_helper.h"

#include "xiaodb/convenience.h"

namespace XIAODB_NAMESPACE
{

    ConfigOptions::ConfigOptions() : registry(ObjectRegistry::NewInstance())
    {
        env = Env::Default();
    }

    ConfigOptions::ConfigOptions(const DBOptions &db_opts) : env(dp_opts.env)
    {
        registry = ObjectRegistry::newInstance();
    }
}