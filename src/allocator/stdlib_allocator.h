#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include <stdlib.h>

struct StdlibAllocator final : IAllocator
{
    StdlibAllocator(AllocType type) : IAllocator(0), m_type(type) {}
    ~StdlibAllocator() {}

    AllocType m_type;

    void Clear() final { }

    void* Alloc(i32 bytes) final
    {
        using namespace Allocator;

        if (bytes > 0)
        {
            bytes = AlignBytes(bytes);
            void* ptr = malloc(bytes);
            return MakePtr(ptr, m_type, bytes);
        }
        ASSERT(bytes == 0);
        return nullptr;
    }

    void* Realloc(void* ptr, i32 bytes) final
    {
        using namespace Allocator;

        if (!ptr)
        {
            return Alloc(bytes);
        }
        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            Free(ptr);
            return nullptr;
        }
        bytes = AlignBytes(bytes);
        Header* hdr = ToHeader(ptr, m_type);
        const i32 rc = Load(hdr->refcount, MO_Relaxed);
        ASSERT(rc == 1);
        void* pNew = realloc(hdr, bytes);
        return MakePtr(pNew, m_type, bytes);
    }

    void Free(void* ptr) final
    {
        using namespace Allocator;

        if (ptr)
        {
            Header* hdr = ToHeader(ptr, m_type);
            const i32 rc = Dec(hdr->refcount, MO_Relaxed);
            ASSERT(rc == 1);
            free(hdr);
        }
    }
};
