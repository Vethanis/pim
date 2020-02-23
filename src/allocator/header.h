#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"
#include <string.h>

namespace Allocator
{
    static constexpr i32 kAlign = 16;
    static constexpr i32 kMask = kAlign - 1;

    struct Header
    {
        i32 type;
        i32 size;
        i32 offset;
        i32 arg1;
    };
    SASSERT((sizeof(Header) & kMask) == 0);

    static constexpr i32 kPadBytes = sizeof(Header) + kAlign;

    static i64 Align(i64 x, i64& offset)
    {                                           // ex:
        const i64 rem = x & kMask;              // 21 % 16 = 5
        const i64 fix = (kAlign - rem) & kMask; // 16 - 5 = 11
        x += fix;                               // 21 + 11 = 32
        offset = fix;
        return x;
    }

    static i32 AlignBytes(i32 x)
    {
        ASSERT(x > 0);
        i64 offset = 0;
        const i32 y = (i32)Align(x + kPadBytes, offset);
        ASSERT(y > 0);
        return y;
    }

    static void* AlignPtr(void* ptr, i64& offset)
    {
        ASSERT(ptr);
        return (void*)Align((i64)ptr, offset);
    }

    static Header* RawToHeader(void* pRaw, AllocType type, i32 size, i32 arg1 = 0)
    {
        i64 offset = 0;
        Header* hdr = (Header*)AlignPtr(pRaw, offset);
        hdr->type = type;
        hdr->size = size;
        hdr->offset = (i32)offset;
        hdr->arg1 = arg1;
        return hdr;
    }

    static void* HeaderToRaw(Header* hdr)
    {
        ASSERT(hdr);
        const i32 offset = hdr->offset;
        ASSERT(offset >= 0);
        ASSERT(offset < kAlign);
        u8* pBytes = (u8*)hdr;
        return pBytes - offset;
    }

    static void* HeaderToUser(Header* hdr)
    {
        ASSERT(hdr);
        return hdr + 1;
    }

    static Header* UserToHeader(void* pUser, AllocType type)
    {
        ASSERT(pUser);
        Header* header = (Header*)pUser;
        header -= 1;
        ASSERT(header->type == type);
        ASSERT(header->size > 0);
        ASSERT(header->offset >= 0);
        ASSERT(header->offset < kAlign);
        return header;
    }

    static void* RawToUser(void* pRaw, AllocType type, i32 size, i32 arg1 = 0)
    {
        Header* pHeader = RawToHeader(pRaw, type, size, arg1);
        void* pUser = HeaderToUser(pHeader);
        return pUser;
    }

    static void* UserToRaw(void* pUser, AllocType type)
    {
        Header* pHeader = UserToHeader(pUser, type);
        void* pRaw = HeaderToRaw(pHeader);
        return pRaw;
    }

    static void Copy(Header* dst, Header* src)
    {
        void* pDst = HeaderToUser(dst);
        const void* pSrc = HeaderToUser(src);
        const i32 bytes = Min(dst->size, src->size) - kPadBytes;
        if (bytes > 0)
        {
            memcpy(pDst, pSrc, bytes);
        }
    }
};
