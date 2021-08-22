#include "io/env.h"

#include "common/stringutil.h"
#include <stdlib.h>

static i32 NotNeg(i32 x)
{
    ASSERT(x >= 0);
    return x;
}

static void* NotNull(void* x)
{
    ASSERT(x != NULL);
    return x;
}

static i32 IsZero(i32 x)
{
    ASSERT(x == 0);
    return x;
}

bool Env_Search(const char* filename, const char* varname, char* dst)
{
    ASSERT(filename);
    ASSERT(varname);
    ASSERT(dst);
    dst[0] = 0;
    _searchenv(filename, varname, dst);
    return dst[0] != 0;
}

const char* Env_Get(const char* varname)
{
    ASSERT(varname);
    return (const char*)NotNull(getenv(varname));
}

bool Env_Set(const char* varname, const char* value)
{
    ASSERT(varname);
    char buf[260] = { 0 };
    SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
    return IsZero(_putenv(buf)) == 0;
}
