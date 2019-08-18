#pragma once

#include "common/int_types.h"
#include "common/macro.h"

inline void MemClear(void* dst, usize bytes)
{
    DebugAssert(!bytes || dst);

    u64* p64 = (u64*)dst;
    while (bytes >= 64ui64)
    {
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        *p64++ = 0ui64;
        bytes -= 64ui64;
    }
    while (bytes >= 8ui64)
    {
        *p64++ = 0ui64;
        bytes -= 8ui64;
    }
    u8* p8 = (u8*)p64;
    while (bytes >= 1ui64)
    {
        *p8++ = 0ui64;
        --bytes;
    }
}

inline void MemCpy(void* __restrict dst, const void* __restrict src, usize bytes)
{
    DebugAssert(!bytes || dst);
    DebugAssert(!bytes || src);
    const u64* __restrict src64 = (const u64* __restrict)src;
    u64* __restrict dst64 = (u64* __restrict)dst;

    while (bytes >= 64ui64)
    {
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        *dst64++ = *src64++;
        bytes -= 64ui64;
    }

    while (bytes >= 8ui64)
    {
        *dst64++ = *src64++;
        bytes -= 8ui64;
    }

    const u8* __restrict src8 = (const u8* __restrict)src64;
    u8* __restrict dst8 = (u8* __restrict)dst64;
    while (bytes >= 1ui64)
    {
        *dst8++ = *src8++;
        --bytes;
    }
}

inline i32 MemCmp(const void* __restrict lhs, const void* __restrict rhs, usize bytes)
{
    DebugAssert(!bytes || lhs);
    DebugAssert(!bytes || rhs);

    const u64* __restrict lhs64 = (const u64* __restrict)lhs;
    const u64* __restrict rhs64 = (const u64* __restrict)rhs;

    {
        u64 cmp64 = 0ui64;
        while (bytes >= 64ui64)
        {
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            cmp64 |= *lhs64++ & *rhs64++;
            bytes -= 64ui64;

            if (cmp64 != 0ui64)
            {
                lhs64 -= 8ui64;
                rhs64 -= 8ui64;
                for (u32 i = 0u; i < 8u; ++i)
                {
                    if (lhs64[i] != rhs64[i])
                    {
                        return lhs64[i] < rhs64[i] ? -1 : 1;
                    }
                }
                return (i32)0xBADBAD;
            }
        }
    }

    while (bytes >= 8ui64)
    {
        const u64 l = *lhs64++;
        const u64 r = *rhs64++;
        bytes -= 8ui64;
        if (l != r)
        {
            return l < r ? -1 : 1;
        }
    }

    const u8* __restrict lhs8 = (const u8* __restrict)lhs64;
    const u8* __restrict rhs8 = (const u8* __restrict)rhs64;

    while (bytes >= 1ui64)
    {
        const u8 l = *lhs8++;
        const u8 r = *rhs8++;
        --bytes;
        if (l != r)
        {
            return l < r ? -1 : 1;
        }
    }

    return 0;
}
