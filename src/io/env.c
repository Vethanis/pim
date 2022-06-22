#include "io/env.h"

#include "common/stringutil.h"
#include <stdlib.h>

#if PLAT_WINDOWS
#ifndef putenv
#   define putenv(p)   _putenv((p))
#endif // putenv
#endif // PLAT_WINDOWS

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

const char* Env_Get(const char* varname)
{
    ASSERT(varname);
    return NotNull(getenv(varname));
}

bool Env_Set(const char* varname, const char* value)
{
    ASSERT(varname);
    char buf[260] = { 0 };
    SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
    return IsZero(putenv(buf)) == 0;
}
