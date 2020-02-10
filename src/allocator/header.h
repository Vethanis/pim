#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"
#include "os/atomics.h"

namespace Allocator
{
    struct Header
    {
        i32 type;
        i32 size;
        i32 refcount;
        i32 arg1;

        Slice<u8> AsRaw()
        {
            ASSERT(this);
            return { (u8*)this, size };
        }

        Slice<u8> AsUser()
        {
            ASSERT(this);
            Header* ptr = this + 1;
            return { (u8*)ptr, size - (i32)sizeof(Header) };
        }

        u8* begin() { return AsRaw().begin(); }
        u8* end() { return AsRaw().end(); }
    };

    SASSERT(sizeof(Header) == 16);

    static Header* ToHeader(void* ptr, AllocType type)
    {
        ASSERT(ptr);
        Header* header = (Header*)ptr;
        header -= 1;
        ASSERT(header->type == type);
        ASSERT(header->size > 0);
        return header;
    }

    static void* ToPtr(Header* hdr)
    {
        ASSERT(hdr);
        return hdr + 1;
    }

    static void* MakePtr(void* pRaw, AllocType type, i32 size, i32 arg1 = 0)
    {
        ASSERT(pRaw);
        Header* hdr = (Header*)pRaw;
        Store(hdr->type, type, MO_Relaxed);
        Store(hdr->size, size, MO_Relaxed);
        Store(hdr->refcount, 1, MO_Relaxed);
        Store(hdr->arg1, arg1, MO_Relaxed);
        return hdr + 1;
    }

    static void Copy(Header* dst, Header* src)
    {
        ::Copy(dst->AsUser(), src->AsUser());
    }

    static constexpr i32 kAlign = 64;
    static constexpr i32 kMask = kAlign - 1;

    static i32 AlignBytes(i32 x)
    {
        ASSERT(x > 0);
        x += sizeof(Header);
        i32 rem = x & kMask;
        i32 fix = (kAlign - rem) & kMask;
        i32 y = x + fix;
        ASSERT((y & kMask) == 0);
        ASSERT(y > 0);
        return y;
    }
};
