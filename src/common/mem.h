#pragma once

#include "common/macro.h"
#include "common/int_types.h"

inline void pimcpy(void * Restrict dst, const void * Restrict src, usize nBytes)
{
    DebugAssert((dst && src) || !nBytes);
    DebugAssert((dst != src) || !nBytes);

    u64* Restrict dst8 = (u64* Restrict)dst;
    const u64* Restrict src8 = (const u64* Restrict)src;
    while (nBytes >= 64u)
    {
        nBytes -= 64u;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
    }
    while (nBytes >= 8u)
    {
        nBytes -= 8u;
        *dst8++ = *src8++;
    }
    u8* Restrict dst1 = (u8* Restrict)dst8;
    const u8* Restrict src1 = (const u8* Restrict)src8;
    while (nBytes-- >= 1u)
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
        nBytes -= 64u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
        *dst8++ = 0u;
    }
    while (nBytes >= 8u)
    {
        nBytes -= 8u;
        *dst8++ = 0u;
    }
    u8* Restrict dst1 = (u8* Restrict)dst8;
    while (nBytes-- >= 1u)
    {
        *dst1++ = 0u;
    }
}

inline i32 pimcmp(void* Restrict lhs, void* Restrict rhs, usize nBytes)
{
    DebugAssert((lhs && rhs) || !nBytes);
    DebugAssert((lhs != rhs) || !nBytes);

    const u64* Restrict lhs8 = (const u64* Restrict)lhs;
    const u64* Restrict rhs8 = (const u64* Restrict)rhs;

    while (nBytes >= 64u)
    {
        u64 cmp8 = 0;
        for (i32 i = 0; i < 8; ++i)
        {
            cmp8 |= lhs8[i] - rhs8[i];
        }
        if (cmp8 != 0)
        {
            break;
        }

        lhs8 += 8;
        rhs8 += 8;
        nBytes -= 64u;
    }

    while (nBytes >= 8u)
    {
        nBytes -= 8u;
        const u64 a = *lhs8++;
        const u64 b = *rhs8++;
        if (a != b)
        {
            return a < b ? -1 : 1;
        }
    }

    const u8* Restrict lhs1 = (const u8* Restrict)lhs8;
    const u8* Restrict rhs1 = (const u8* Restrict)rhs8;
    while (nBytes-- >= 1u)
    {
        const u8 a = *lhs1++;
        const u8 b = *rhs1++;
        if (a != b)
        {
            return a < b ? -1 : 1;
        }
    }

    return 0;
}
