#include "common/stringutil.h"

#include <stdio.h>

char* VSPrintf(char* dst, isize size, cstr fmt, VaList va)
{
    dst[size - 1] = 0;
    fmt = fmt ? fmt : "(NULL)";
    i32 ct = vsnprintf(dst, size - 1, fmt, va);
    ct = Max(0, ct);
    dst[ct] = 0;
    dst[size - 1] = 0;
    return dst + ct;
}

