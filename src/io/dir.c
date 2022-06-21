#include "io/dir.h"

#include <direct.h>
#include <io.h>

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

bool IO_GetCwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(_getcwd(dst, size - 1)) != NULL;
}

bool IO_ChDir(const char* path)
{
    ASSERT(path);
    return IsZero(_chdir(path)) == 0;
}

bool IO_MkDir(const char* path)
{
    ASSERT(path);
    return _mkdir(path) == 0;
}

bool IO_RmDir(const char* path)
{
    ASSERT(path);
    return IsZero(_rmdir(path)) == 0;
}

bool IO_ChMod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(_chmod(path, flags)) == 0;
}
