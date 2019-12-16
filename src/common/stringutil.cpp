#include "common/stringutil.h"

#include <stdio.h>

void VSPrintf(char* dst, i32 size, cstr fmt, VaList va)
{
    ASSERT((dst && fmt) || !size);
    ASSERT(size >= 0);
    ASSERT(va);

    if (size > 0)
    {
        const i32 back = size - 1;
        vsnprintf(dst, back, fmt ? fmt : "(NULL)", va);
        dst[back] = 0;
    }
}
