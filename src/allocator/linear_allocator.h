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
        u8* tail = LoadPtr(m_tail);
        u8* head = LoadPtr(m_head);
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

    void Free(void* ptr) final
    {
        using namespace Allocator;

        if (!ptr)
        {
            return;
        }

        Header* prev = ToHeader(ptr, Alloc_Linear);
        u8* begin = prev->begin();
        u8* end = prev->end();

    tryfree:
        u8* head = LoadPtr(m_head);
        ASSERT(head);
        if (end == head)
        {
            if (!CmpExStrongPtr(m_head, head, begin, MO_Release))
            {
                goto tryfree;
            }
        }
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

        const i32 userBytes = bytes;
        bytes = AlignBytes(bytes);

        Header* prev = ToHeader(ptr, Alloc_Linear);
        const i32 diff = bytes - prev->size;
        u8* end = prev->end();

    tryresize:
        u8* head = LoadPtr(m_head);
        ASSERT(head);
        if (end == head)
        {
            if (CmpExStrongPtr(m_head, head, head + diff))
            {
                prev->size += diff;
                return prev;
            }
            goto tryresize;
        }

        void* pNew = Alloc(userBytes);
        if (pNew)
        {
            Copy(ToHeader(pNew, Alloc_Linear), prev);
        }
        Free(prev);
        return pNew;
    }

private:
    u8* m_ptr;
    u8* m_head;
    u8* m_tail;
};
