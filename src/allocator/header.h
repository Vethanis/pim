#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"
#include <string.h>

namespace Allocator
{
    static constexpr i32 kAlign = 16;
    static constexpr i32 kMask = kAlign - 1;

    struct alignas(kAlign) Header
    {
        i32 type;
        i32 size;
        i32 offset;
        i32 arg1;
        Header* prev;
        Header* next;
    };
    SASSERT((sizeof(Header) & kMask) == 0);

    static constexpr i32 kPadBytes = sizeof(Header);

    static bool IsAligned(void* ptr)
    {
        isize addr = (isize)ptr;
        return (addr & kMask) == 0;
    }

    static i32 Align(i32 x)
    {
        const i32 rem = x & kMask;
        const i32 fix = (kAlign - rem) & kMask;
        return x + fix;
    }

    static i32 AlignBytes(i32 x)
    {
        ASSERT(x > 0);
        const i32 y = Align(x + kPadBytes);
        ASSERT(y > 0);
        ASSERT((y & kMask) == 0);
        return y;
    }

    static Header* RawToHeader(void* pRaw, AllocType type, i32 size, i32 arg1 = 0)
    {
        ASSERT(pRaw);
        ASSERT(IsAligned(pRaw));
        Header* hdr = (Header*)pRaw;
        hdr->type = type;
        hdr->size = size;
        hdr->arg1 = arg1;
        return hdr;
    }

    static void* HeaderToRaw(Header* hdr)
    {
        ASSERT(hdr);
        ASSERT(IsAligned(hdr));
        return hdr;
    }

    static void* HeaderToUser(Header* hdr)
    {
        ASSERT(hdr);
        ASSERT(IsAligned(hdr));
        return hdr + 1;
    }

    static Header* UserToHeader(void* pUser, AllocType type)
    {
        ASSERT(pUser);
        ASSERT(IsAligned(pUser));
        Header* header = (Header*)pUser - 1;
        ASSERT(header->type == type);
        ASSERT(header->size > 0);
        return header;
    }

    static void* RawToUser(void* pRaw, AllocType type, i32 size, i32 arg1 = 0)
    {
        return HeaderToUser(RawToHeader(pRaw, type, size, arg1));
    }

    static void* UserToRaw(void* pUser, AllocType type)
    {
        return HeaderToRaw(UserToHeader(pUser, type));
    }

    static void Copy(Header* dst, Header* src)
    {
        ASSERT(dst);
        ASSERT(src);
        void* pDst = HeaderToUser(dst);
        const void* pSrc = HeaderToUser(src);
        const i32 bytes = Min(dst->size, src->size) - kPadBytes;
        if (bytes > 0)
        {
            memmove(pDst, pSrc, bytes);
        }
    }
};
