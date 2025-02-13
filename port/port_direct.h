#pragma once

#ifdef XIAODB_PLATFORM_POSIX
#include <dirent.h>
#include <sys/types.h>
#elif defined(OS_WIN)

namespace XIAODB_NAMESPACE
{
    namespace port
    {

        struct dirent
        {
            char d_name[_MAX_PATH];
        };

        struct DIR;

        DIR *opendir(const char *name);

        dirent *readdir(DIR *dirp);

        int closedir(DIR *dirp);
    }

    using port::closedir;
    using port::DIR;
    using port::dirent;
    using port::opendir;
    using port::readdir;
}

#endif