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

bool pim_getcwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(_getcwd(dst, size - 1)) != NULL;
}

bool pim_chdir(const char* path)
{
    ASSERT(path);
    return IsZero(_chdir(path)) == 0;
}

bool pim_mkdir(const char* path)
{
    ASSERT(path);
    return IsZero(_mkdir(path)) == 0;
}

bool pim_rmdir(const char* path)
{
    ASSERT(path);
    return IsZero(_rmdir(path)) == 0;
}

bool pim_chmod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(_chmod(path, flags)) == 0;
}
