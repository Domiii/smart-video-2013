#ifndef UTIL_FILEUTIL_H
#define UTIL_FILEUTIL_H

#include "Util.h"


#ifdef _WIN32
    #include <direct.h>

    #define mkdir(fname, privs) _mkdir(fname)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

namespace Util
{
    void MkDir(std::string fname, int mode = 0777) 
    {
        mkdir(fname.c_str(), mode);
    }
}

#endif // UTIL_FILEUTIL_H