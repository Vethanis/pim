#include "io/dir.h"

#include <direct.h>
#include <io.h>

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

bool Dir_Get(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(_getcwd(dst, size - 1)) != NULL;
}

bool Dir_Set(const char* path)
{
    ASSERT(path);
    return IsZero(_chdir(path)) == 0;
}

bool Dir_Add(const char* path)
{
    ASSERT(path);
    return IsZero(_mkdir(path)) == 0;
}

bool Dir_Rm(const char* path)
{
    ASSERT(path);
    return IsZero(_rmdir(path)) == 0;
}

bool Dir_ChMod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(_chmod(path, flags)) == 0;
}
