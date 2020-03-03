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
    void Init(i32 capacity, AllocType type) final
    {
        ASSERT(capacity > 0);
        void* memory = malloc(capacity);
        ASSERT(memory);
        m_type = type;
        m_ptr = (u8*)memory;
        m_head = m_ptr;
        m_tail = m_head + capacity;
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

    void* Alloc(i32 userBytes) final
    {
        using namespace Allocator;

        const i32 rawBytes = AlignBytes(userBytes);
        u8* tail = LoadPtr(m_tail, MO_Relaxed);

    tryalloc:
        u8* head = LoadPtr(m_head, MO_Relaxed);
        if (head + rawBytes <= tail)
        {
            if (CmpExStrongPtr(m_head, head, head + rawBytes, MO_Acquire))
            {
                return RawToUser(head, m_type, rawBytes);
            }
            goto tryalloc;
        }
        return 0;
    }

    void Free(void* pUser) final
    {
        using namespace Allocator;

        Header* pHeader = UserToHeader(pUser, m_type);
        void* pRaw = HeaderToRaw(pHeader);

        u8* begin = (u8*)pRaw;
        u8* end = begin + pHeader->size;

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

    void* Realloc(void* pOldUser, i32 userBytes) final
    {
        using namespace Allocator;

        const i32 rawBytes = AlignBytes(userBytes);
        Header* pOldHeader = UserToHeader(pOldUser, m_type);
        const i32 diff = rawBytes - pOldHeader->size;

        if (diff <= 0)
        {
            return pOldUser;
        }

        u8* begin = (u8*)HeaderToRaw(pOldHeader);
        u8* end = begin + pOldHeader->size;

        u8* head = LoadPtr(m_head, MO_Relaxed);
        ASSERT(head);
        if (end == head)
        {
            if (CmpExStrongPtr(m_head, head, head + diff, MO_Acquire))
            {
                pOldHeader->size += diff;
                return pOldUser;
            }
        }

        void* pNewUser = Alloc(userBytes);
        if (pNewUser)
        {
            Copy(UserToHeader(pNewUser, m_type), pOldHeader);
        }
        Free(pOldUser);

        return pNewUser;
    }
};
