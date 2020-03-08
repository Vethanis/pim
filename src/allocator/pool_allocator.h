#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "containers/slice.h"
#include <stdlib.h>

struct PoolAllocator final : IAllocator
{
private:
    using Hunk = Allocator::Header;

    // todo: split pool into 3 regions
    // <= 1kb per alloc
    // <= 64kb per alloc
    // > 64 kb per alloc
    // in order to reduce fragmentation from persistent,
    // small allocations sitting in a 'desert'
    Hunk* m_head;
    Hunk* m_recent;
    i32 m_capacity;
    AllocType m_type;

public:
    void Init(i32 capacity, AllocType type) final
    {
        ASSERT(capacity > 0);
        m_head = (Hunk*)malloc(capacity);
        ASSERT(m_head);
        m_recent = 0;
        m_capacity = capacity;
        m_type = type;

        Clear();
    }

    void Reset() final
    {
        free(m_head);
        m_head = 0;
        m_recent = 0;
        m_capacity = 0;
    }

    void Clear() final
    {
        m_recent = m_head;
        m_head->next = 0;
        m_head->prev = 0;
        m_head->offset = kHunkSize;
        m_head->size = m_capacity - kHunkSize;
        m_head->type = m_type;
        m_head->arg1 = 0;
    }

    void* Alloc(i32 bytes) final
    {
        return _Alloc(bytes);
    }

    void Free(void* ptr) final
    {
        _Free(ptr);
    }

    void* Realloc(void* ptr, i32 bytes) final
    {
        return _Realloc(ptr, bytes);
    }

private:

    static constexpr i32 kHunkSize = (i32)sizeof(Hunk);

    static Hunk* OffsetPtr(void* ptr, i32 offset)
    {
        ASSERT(ptr);
        ASSERT(Allocator::IsAligned(ptr));
        ptr = ((u8*)ptr) + offset;
        ASSERT(Allocator::IsAligned(ptr));
        return (Hunk*)ptr;
    }

    static Hunk* Split(Hunk* pLhs, i32 bytes)
    {
        ASSERT(pLhs);
        ASSERT(!pLhs->arg1);

        const i32 prevSize = pLhs->size;
        const i32 lhs = bytes;
        const i32 rhs = (prevSize - bytes) - kHunkSize;
        ASSERT(rhs > kHunkSize);
        ASSERT(!(lhs & 15));
        ASSERT(!(rhs & 15));

        const i32 offset = kHunkSize + lhs;
        Hunk* pRhs = OffsetPtr(pLhs, offset);
        pRhs->next = 0;
        pRhs->prev = 0;
        pRhs->size = rhs;
        pRhs->offset = pLhs->offset + offset;
        pRhs->arg1 = 0;
        pRhs->type = pLhs->type;

        Hunk* pNext = pLhs->next;
        if (pNext)
        {
            pRhs->next = pNext;
            pNext->prev = pRhs;
        }
        pRhs->prev = pLhs;

        pLhs->next = pRhs;
        pLhs->size = lhs;

        return pRhs;
    }

    static Hunk* Merge(Hunk* pHunk)
    {
        ASSERT(pHunk);
        ASSERT(!pHunk->arg1);

        Hunk* pNext = pHunk->next;
        if (pNext && !pNext->arg1)
        {
            pHunk->size += kHunkSize + pNext->size;

            Hunk* pNext2 = pNext->next;
            pHunk->next = pNext2;
            if (pNext2)
            {
                pNext2->prev = pHunk;
            }
        }

        Hunk* pPrev = pHunk->prev;
        if (pPrev && !pPrev->arg1)
        {
            return Merge(pPrev);
        }

        return pHunk;
    }

    void* _Alloc(i32 userBytes)
    {
        using namespace Allocator;

        const i32 rawBytes = Align(userBytes);

        Hunk* pHunk = m_recent;
    search:
        while (pHunk)
        {
            const i32 size = pHunk->size * (1 - pHunk->arg1);
            const i32 diff = size - rawBytes;
            if (diff >= 0)
            {
                if (diff >= 1024)
                {
                    Split(pHunk, rawBytes);
                }
                pHunk->arg1 = 1;
                pHunk->type = m_type;
                m_recent = pHunk->next;
                return HeaderToUser(pHunk);
            }
            pHunk = pHunk->next;
        }

        if (m_recent != m_head)
        {
            m_recent = m_head;
            pHunk = m_head;
            goto search;
        }

        ASSERT(false);
        return nullptr;
    }

    void _Free(void* pUser)
    {
        using namespace Allocator;

        Hunk* pHunk = UserToHeader(pUser, m_type);
        ASSERT(pHunk->arg1);
        ASSERT(pHunk->type == m_type);
        pHunk->arg1 = 0;
        m_recent = Merge(pHunk);
    }

    void* _Realloc(void* pOldUser, i32 userBytes)
    {
        using namespace Allocator;

        Hunk* pHunk = UserToHeader(pOldUser, m_type);
        if (userBytes <= pHunk->size)
        {
            return pOldUser;
        }

        _Free(pOldUser);
        void* pNewUser = _Alloc(userBytes);
        if (!pNewUser)
        {
            return nullptr;
        }

        if (pNewUser != pOldUser)
        {
            Hunk* pOld = OffsetPtr(pOldUser, -kHunkSize);
            Hunk* pNew = OffsetPtr(pNewUser, -kHunkSize);
            memmove(pNewUser, pOldUser, Min(pOld->size, pNew->size));
        }

        return pNewUser;
    }
};
