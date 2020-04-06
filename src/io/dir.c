#include "io/dir.h"

#include <direct.h>
#include <io.h>

static pim_thread_local i32 ms_errno;

i32 dir_errno(void)
{
    i32 rv = ms_errno;
    ms_errno = 0;
    return rv;
}

static i32 NotNeg(i32 x)
{
    if (x < 0)
    {
        ms_errno = 1;
    }
    return x;
}

static void* NotNull(void* x)
{
    if (!x)
    {
        ms_errno = 1;
    }
    return x;
}

static i32 IsZero(i32 x)
{
    if (x)
    {
        ms_errno = 1;
    }
    return x;
}

void pim_getcwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    NotNull(_getcwd(dst, size - 1));
}

void pim_chdir(const char* path)
{
    ASSERT(path);
    IsZero(_chdir(path));
}

void pim_mkdir(const char* path)
{
    ASSERT(path);
    IsZero(_mkdir(path));
}

void pim_rmdir(const char* path)
{
    ASSERT(path);
    IsZero(_rmdir(path));
}

void pim_chmod(const char* path, i32 flags)
{
    ASSERT(path);
    IsZero(_chmod(path, flags));
}
