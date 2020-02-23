#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "os/atomics.h"
#include <stdlib.h>

struct LinearAllocator final : IAllocator
{
private:
    u8* m_ptr;
    u8* m_head;
    u8* m_tail;
    AllocType m_type;

public:
    void Init(i32 bytes, AllocType type) final
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);
        m_type = type;
        m_ptr = (u8*)memory;
        m_head = m_ptr;
        m_tail = m_head + bytes;
    }

    void Reset() final
    {
        void* ptr = m_ptr;
        m_ptr = 0;
        m_head = 0;
        m_tail = 0;
        free(ptr);
    }

    void Clear() final
    {
        StorePtr(m_head, m_ptr);
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
                return MakePtr(head, m_type, bytes);
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

        Header* hOld = ToHeader(pOld, m_type);
        ASSERT(Dec(hOld->refcount) == 1);

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

        Header* hOld = ToHeader(pOld, m_type);
        ASSERT(Load(hOld->refcount) == 1);

        const i32 diff = bytes - hOld->size;
        if (diff <= 0)
        {
            return pOld;
        }

        u8* end = hOld->end();
        u8* head = LoadPtr(m_head, MO_Relaxed);
        ASSERT(head);
        if (end == head)
        {
            if (CmpExStrongPtr(m_head, head, head + diff, MO_Acquire))
            {
                hOld->size += diff;
                return pOld;
            }
        }

        void* pNew = Alloc(userBytes);
        if (pNew)
        {
            Copy(ToHeader(pNew, m_type), hOld);
        }
        Free(pOld);
        return pNew;
    }
};
