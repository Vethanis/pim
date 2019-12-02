#pragma once

#include "common/macro.h"
#include "common/unroll.h"

inline i32 MemCmp(
    const void* Restrict lhs,
    const void* Restrict rhs,
    i32 bytes)
{
    Assert(bytes >= 0);
    Assert(!bytes || (lhs && rhs));

    i32 n64, n8, n1;
    Unroll64(bytes, n64, n8, n1);

    const u64* Restrict l8 = (const u64* Restrict)lhs;
    const u64* Restrict r8 = (const u64* Restrict)rhs;
    for (i32 i = 0; i < n64; ++i)
    {
        u64 cmp = 0;
        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;

        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;
        cmp |= *l8++ - *r8++;

        if (cmp != 0)
        {
            l8 -= 8;
            r8 -= 8;
            n8 += 8;
            break;
        }
    }

    for (i32 i = 0; i < n8; ++i)
    {
        u64 L = *l8++;
        u64 R = *r8++;
        if (L != R)
        {
            return L < R ? -1 : 1;
        }
    }

    const u8* Restrict l1 = (const u8* Restrict)l8;
    const u8* Restrict r1 = (const u8* Restrict)r8;
    for (i32 i = 0; i < n1; ++i)
    {
        i32 cmp = *l1 - *r1;
        if (cmp)
        {
            return cmp;
        }
    }

    return 0;
}

inline void MemCpy(
    void* Restrict dst,
    const void* Restrict src,
    i32 bytes)
{
    Assert(bytes >= 0);
    Assert(!bytes || (dst && src));

    i32 n64, n8, n1;
    Unroll64(bytes, n64, n8, n1);

    u64* Restrict dst8 = (u64* Restrict)dst;
    const u64* Restrict src8 = (const u64* Restrict)src;
    for (i32 i = 0; i < n64; ++i)
    {
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;

        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
        *dst8++ = *src8++;
    }

    for (i32 i = 0; i < n8; ++i)
    {
        *dst8++ = *src8++;
    }

    u8* Restrict dst1 = (u8* Restrict)dst8;
    const u8* Restrict src1 = (const u8* Restrict)src8;
    for (i32 i = 0; i < n1; ++i)
    {
        *dst1++ = *src1++;
    }
}

inline void MemClear(void* dst, i32 bytes)
{
    Assert(bytes >= 0);
    Assert(!bytes || dst);

    i32 n64, n8, n1;
    Unroll64(bytes, n64, n8, n1);

    u64* dst8 = (u64*)dst;
    for (i32 i = 0; i < n64; ++i)
    {
        *dst8++ = 0;
        *dst8++ = 0;
        *dst8++ = 0;
        *dst8++ = 0;

        *dst8++ = 0;
        *dst8++ = 0;
        *dst8++ = 0;
        *dst8++ = 0;
    }

    for (i32 i = 0; i < n8; ++i)
    {
        *dst8++ = 0;
    }

    u8* dst1 = (u8*)dst8;
    for (i32 i = 0; i < n1; ++i)
    {
        *dst1++ = 0;
    }
}

inline void MemSet(void* dst, u8 value, i32 bytes)
{
    if (value == 0)
    {
        MemClear(dst, bytes);
    }

    Assert(bytes >= 0);
    Assert(!bytes || dst);

    u64 value8 = 0;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;
    value8 = (value8 << 8) | value;

    i32 n64, n8, n1;
    Unroll64(bytes, n64, n8, n1);

    u64* dst8 = (u64*)dst;
    for (i32 i = 0; i < n64; ++i)
    {
        *dst8++ = value8;
        *dst8++ = value8;
        *dst8++ = value8;
        *dst8++ = value8;

        *dst8++ = value8;
        *dst8++ = value8;
        *dst8++ = value8;
        *dst8++ = value8;
    }

    for (i32 i = 0; i < n8; ++i)
    {
        *dst8++ = value8;
    }

    u8* dst1 = (u8*)dst8;
    for (i32 i = 0; i < n1; ++i)
    {
        *dst1++ = value;
    }
}
