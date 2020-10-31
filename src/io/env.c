#include "io/env.h"

#include <stdlib.h>
#include "common/stringutil.h"

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

bool pim_searchenv(const char* filename, const char* varname, char* dst)
{
    ASSERT(filename);
    ASSERT(varname);
    ASSERT(dst);
    dst[0] = 0;
    _searchenv(filename, varname, dst);
    return dst[0] != 0;
}

const char* pim_getenv(const char* varname)
{
    ASSERT(varname);
    return (const char*)NotNull(getenv(varname));
}

bool pim_putenv(const char* varname, const char* value)
{
    ASSERT(varname);
    char buf[260] = { 0 };
    SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
    return IsZero(_putenv(buf)) == 0;
}
