#include "io/dir.h"

pim_inline void* NotNull(void* x)
{
    ASSERT(x != NULL);
    return x;
}

pim_inline i32 IsZero(i32 x)
{
    ASSERT(x == 0);
    return x;
}

#if PLAT_WINDOWS
#include <direct.h>
#include <io.h>

#ifndef getcwd
#   define getcwd(d, s) _getcwd(d, s)
#endif // getcwd
#ifndef chdir
#   define chdir(p) _chdir(p)
#endif // chdir
#ifndef rmdir
#   define rmdir(p) _rmdir(p)
#endif // rmdir
#ifndef chmod
#   define chmod(p, f) _chmod(p, f)
#endif // chmod

// need special mkdir as linux has a second arg for permissions
#define pim_mkdir(p) _mkdir(p)

#else
#include <unistd.h>
#include <sys/stat.h>

#define pim_mkdir(p) mkdir(p, 0755)

#endif // PLAT_WINDOWS

bool IO_GetCwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(getcwd(dst, size - 1)) != NULL;
}

bool IO_ChDir(const char* path)
{
    ASSERT(path);
    return IsZero(chdir(path)) == 0;
}

bool IO_MkDir(const char* path)
{
    ASSERT(path);
    return pim_mkdir(path) == 0;
}

bool IO_RmDir(const char* path)
{
    ASSERT(path);
    return IsZero(rmdir(path)) == 0;
}

bool IO_ChMod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(chmod(path, flags)) == 0;
}
