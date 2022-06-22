#include "io/env.h"

#include "common/stringutil.h"
#include <stdlib.h>

#if PLAT_WINDOWS
#ifndef putenv
#   define putenv(p)   _putenv(p)
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

bool Env_Search(const char* filename, const char* varname, char* dst)
{
    ASSERT(filename);
    ASSERT(varname);
    ASSERT(dst);
    dst[0] = 0;

    #if PLAT_WINDOWS

    _searchenv(filename, varname, dst);

    #else

    // TODO: unsure how we're meant to safely copy to destination without length.
    // Currently env_search isn't referenced though.
    /*
    struct stat buffer;
    char* env = getenv("PATH");
    do
    {
        char* p = strchr(env, ':');
        if (p != NULL && stat(p, &buffer) == 0)
        {
            break;
        }
        env = p + 1;
    } 
    while (p != NULL);

    if (p != null)
    {
        StrCpy(dst, ???, p);
    }
    */

    #endif // PLAT_WINDOWS


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
    return IsZero(putenv(buf)) == 0;
}
