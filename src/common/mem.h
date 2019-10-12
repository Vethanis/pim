#pragma once

#include "common/macro.h"
#include "common/int_types.h"

inline void pimcpy(void * Restrict dst, const void * Restrict src, usize nBytes)
{
    DebugAssert((dst && src) || !nBytes);

    u64* Restrict dst8 = (u64* Restrict)dst;
    const u64* Restrict src8 = (const u64* Restrict)src;
    while (nBytes >= 64u)
    {
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        nBytes -= 64u;
    }
    while (nBytes >= 8u)
    {
        *dst8++ = *src8++;
        nBytes -= 8u;
    }
    u8* Restrict dst1 = (u8* Restrict)dst8;
    const u8* Restrict src1 = (const u8* Restrict)src8;
    while (nBytes-- >= 1)
    {
        *dst1++ = *src1++;
    }
}

inline void pimclr(void* Restrict dst, usize nBytes)
{
    DebugAssert(dst || !nBytes);

    u64* Restrict dst8 = (u64* Restrict)dst;
    while (nBytes >= 64u)
    {
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        nBytes -= 64u;
    }
    while (nBytes >= 8u)
    {
        *dst8++ = 0u;
        nBytes -= 8u;
    }
    u8* Restrict dst1 = (u8* Restrict)dst8;
    while (nBytes-- >= 1)
    {
        *dst1++ = 0u;
    }
}
