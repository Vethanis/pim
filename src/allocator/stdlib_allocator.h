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

    void* Realloc(void* pOld, i32 bytes) final
    {
        using namespace Allocator;

        if (!pOld)
        {
            return Alloc(bytes);
        }
        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            Free(pOld);
            return nullptr;
        }
        bytes = AlignBytes(bytes);
        Header* hOld = ToHeader(pOld, m_type);
        const i32 rc = Load(hOld->refcount, MO_Relaxed);
        ASSERT(rc == 1);
        Header* hNew = (Header*)realloc(hOld, bytes);
        return MakePtr(hNew, m_type, bytes);
    }

    void Free(void* pOld) final
    {
        using namespace Allocator;

        if (pOld)
        {
            Header* hOld = ToHeader(pOld, m_type);
            const i32 rc = Dec(hOld->refcount, MO_Relaxed);
            ASSERT(rc == 1);
            free(hOld);
        }
    }
};
