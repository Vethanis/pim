#define _CRT_SECURE_NO_WARNINGS
#include "io/env.h"

#include <stdlib.h>
#include "common/stringutil.h"

static PIM_TLS int32_t ms_errno;

int32_t env_errno(void)
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

void pim_searchenv(const char* filename, const char* varname, char* dst)
{
    ASSERT(filename);
    ASSERT(varname);
    ASSERT(dst);
    dst[0] = 0;
    _searchenv(filename, varname, dst);
    if (dst[0] == 0)
    {
        ms_errno = 1;
    }
}

const char* pim_getenv(const char* varname)
{
    ASSERT(varname);
    return (const char*)NotNull(getenv(varname));
}

void pim_putenv(const char* varname, const char* value)
{
    ASSERT(varname);
    char buf[260];
    SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
    IsZero(_putenv(buf));
}
