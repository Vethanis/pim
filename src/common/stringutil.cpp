#include "common/stringutil.h"

#include <stdio.h>

void VSPrintf(char* dst, i32 size, cstr fmt, VaList va)
{
    Assert((dst && fmt) || !size);
    Assert(size >= 0);
    Assert(va);

    if (size > 0)
    {
        const i32 back = size - 1;
        vsnprintf(dst, back, fmt ? fmt : "(NULL)", va);
        dst[back] = 0;
    }
}
