#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "os/atomics.h"
#include <stdlib.h>

struct LinearAllocator final : IAllocator
{
public:
    LinearAllocator(i32 bytes) : IAllocator(bytes)
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);
        StorePtr(m_ptr, (u8*)memory);
        StorePtr(m_head, (u8*)memory);
        StorePtr(m_tail, (u8*)memory + bytes);
    }

    ~LinearAllocator()
    {
        void* ptr = LoadPtr(m_ptr);
        StorePtr(m_tail, (u8*)0);
        StorePtr(m_head, (u8*)0);
        StorePtr(m_ptr, (u8*)0);
        free(ptr);
    }

    void Clear() final
    {
        StorePtr(m_head, LoadPtr(m_ptr));
    }

    void* Alloc(i32 bytes) final
    {
        using namespace Allocator;

        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            return nullptr;
        }

        bytes = AlignBytes(bytes);

    tryalloc:
        u8* tail = LoadPtr(m_tail, MO_Relaxed);
        u8* head = LoadPtr(m_head, MO_Relaxed);
        if (head + bytes <= tail)
        {
            if (CmpExStrongPtr(m_head, head, head + bytes, MO_Acquire))
            {
                return MakePtr(head, Alloc_Linear, bytes);
            }
            goto tryalloc;
        }
        return 0;
    }

    void Free(void* pOld) final
    {
        using namespace Allocator;

        if (!pOld)
        {
            return;
        }

        Header* hOld = ToHeader(pOld, Alloc_Linear);
        const i32 rc = Dec(hOld->refcount, MO_Relaxed);
        ASSERT(rc == 1);

        u8* begin = hOld->begin();
        u8* end = hOld->end();

    tryfree:
        u8* head = LoadPtr(m_head, MO_Relaxed);
        ASSERT(head);
        if (end == head)
        {
            if (!CmpExStrongPtr(m_head, head, begin, MO_Release))
            {
                goto tryfree;
            }
        }
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

        const i32 userBytes = bytes;
        bytes = AlignBytes(bytes);

        Header* hOld = ToHeader(pOld, Alloc_Linear);
        const i32 rc = Load(hOld->refcount, MO_Relaxed);
        ASSERT(rc == 1);

        const i32 diff = bytes - hOld->size;
        if (diff <= 0)
        {
            return pOld;
        }

        u8* end = hOld->end();

    tryresize:
        u8* head = LoadPtr(m_head, MO_Relaxed);
        ASSERT(head);
        if (end == head)
        {
            if (CmpExStrongPtr(m_head, head, head + diff, MO_Acquire))
            {
                hOld->size += diff;
                return pOld;
            }
            goto tryresize;
        }

        void* pNew = Alloc(userBytes);
        if (pNew)
        {
            Copy(ToHeader(pNew, Alloc_Linear), hOld);
        }
        Free(pOld);
        return pNew;
    }

private:
    u8* m_ptr;
    u8* m_head;
    u8* m_tail;
};
