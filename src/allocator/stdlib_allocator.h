#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include <stdlib.h>

struct StdlibAllocator final : IAllocator
{
private:
    AllocType m_type;

public:
    void Init(i32 bytes, AllocType type) final { m_type = type; }
    void Reset() final { }
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
        ASSERT(Load(hOld->refcount) == 1);
        Header* hNew = (Header*)realloc(hOld, bytes);
        return MakePtr(hNew, m_type, bytes);
    }

    void Free(void* pOld) final
    {
        using namespace Allocator;

        if (pOld)
        {
            Header* hOld = ToHeader(pOld, m_type);
            ASSERT(Dec(hOld->refcount) == 1);
            free(hOld);
        }
    }
};
