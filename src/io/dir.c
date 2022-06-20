#include "io/dir.h"

pim_inline i32 NotNeg(i32 x)
{
    ASSERT(x >= 0);
    return x;
}

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

#define pim_getcwd(d, s) _getcwd(d, s)
#define pim_chdir(p) _chdir(p)
#define pim_mkdir(p) _mkdir(p)
#define pim_rmdir(p) _rmdir(p)
#define pim_chmod(p, f) _chmod(p, f)

#else
#include <unistd.h>
#include <sys/stat.h>

#define pim_getcwd(d, s) getcwd(d, s)
#define pim_chdir(p) chdir(p)
#define pim_mkdir(p) mkdir(p, 0755)
#define pim_rmdir(p) rmdir(p)
#define pim_chmod(p, f) chmod(p, f)

#endif // PLAT_WINDOWS

bool IO_GetCwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(pim_getcwd(dst, size - 1)) != NULL;
}

bool IO_ChDir(const char* path)
{
    ASSERT(path);
    return IsZero(pim_chdir(path)) == 0;
}

bool IO_MkDir(const char* path)
{
    ASSERT(path);
    return pim_mkdir(path) == 0;
}

bool IO_RmDir(const char* path)
{
    ASSERT(path);
    return IsZero(pim_rmdir(path)) == 0;
}

bool IO_ChMod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(pim_chmod(path, flags)) == 0;
}
