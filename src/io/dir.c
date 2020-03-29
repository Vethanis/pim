#include "io/dir.h"

#include <direct.h>
#include <io.h>

static PIM_TLS int32_t ms_errno;

int32_t dir_errno(void)
{
    int32_t rv = ms_errno;
    ms_errno = 0;
    return rv;
}

static int32_t NotNeg(int32_t x)
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

static int32_t IsZero(int32_t x)
{
    if (x)
    {
        ms_errno = 1;
    }
    return x;
}

void pim_getcwd(char* dst, int32_t size)
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

void pim_chmod(const char* path, int32_t flags)
{
    ASSERT(path);
    IsZero(_chmod(path, flags));
}
