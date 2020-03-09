#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "containers/slice.h"
#include <stdlib.h>

struct PoolAllocator final : IAllocator
{
private:
    using Hunk = Allocator::Header;

    static constexpr i32 kSmall = 1 << 10;
    static constexpr i32 kMedium = 64 << 10;
    static constexpr i32 kHunkSize = sizeof(Hunk);
    static constexpr i32 kPools = 3;

    struct Pool
    {
        Hunk* pHead;
        Hunk* pRecent;
    };

    Pool m_pools[kPools];
    i32 m_counts[kPools];
    i32 m_used[kPools];
    void* m_ptr;
    i32 m_capacity;
    AllocType m_type;

public:
    void Init(i32 capacity, AllocType type) final
    {
        ASSERT(capacity > 0);
        m_capacity = capacity;
        m_type = type;
        m_ptr = malloc(capacity);
        ASSERT(m_ptr);
        Clear();
    }

    void Reset() final
    {
        Clear();
        free(m_ptr);
        m_ptr = 0;
        m_capacity = 0;
        for (Pool& pool : m_pools)
        {
            pool.pHead = 0;
            pool.pRecent = 0;
        }
    }

    void Clear() final
    {
        u8* pBase = (u8*)m_ptr;
        const i32 trimmed = m_capacity - kPools * Allocator::kAlign;
        const i32 split = Align(trimmed / kPools, Allocator::kAlign);
        i32 offset = 0;
        for (i32 i = 0; i < kPools; ++i)
        {
            u8* ptr = pBase + i * split;
            Hunk* pHead = (Hunk*)ptr;

            pHead->type = m_type;
            pHead->size = split - kHunkSize;
            pHead->offset = offset + kHunkSize;
            pHead->arg1 = 0;
            pHead->prev = 0;
            pHead->next = 0;

            m_pools[i].pHead = pHead;
            m_pools[i].pRecent = 0;
            m_counts[i] = 0;
            m_used[i] = 0;

            offset += split;
        }
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

    merge:
        Hunk* pNext = pHunk->next;
        Hunk* pPrev = pHunk->prev;

        while (pNext && !pNext->arg1)
        {
            pHunk->size += kHunkSize + pNext->size;
            pNext = pNext->next;
            pHunk->next = pNext;
            if (pNext)
            {
                pNext->prev = pHunk;
            }
        }

        if (pPrev && !pPrev->arg1)
        {
            pHunk = pPrev;
            goto merge;
        }

        return pHunk;
    }

    static constexpr i32 SelectPool(i32 bytes)
    {
        if (bytes >= kMedium)
        {
            return 2;
        }
        if (bytes >= kSmall)
        {
            return 1;
        }
        return 0;
    }

    void* _Alloc(i32 userBytes)
    {
        using namespace Allocator;

        const i32 rawBytes = Align(userBytes);
        const i32 iPool = SelectPool(rawBytes);

        Hunk* hunks[] = { m_pools[iPool].pRecent, m_pools[iPool].pHead };
        for (Hunk* pStart : hunks)
        {
            Hunk* pHunk = pStart;
            while (pHunk)
            {
                const i32 size = pHunk->size * (1 - pHunk->arg1);
                const i32 diff = size - rawBytes;
                if (diff >= 0)
                {
                    if (diff >= 256)
                    {
                        Split(pHunk, rawBytes);
                    }
                    pHunk->arg1 = 1;
                    pHunk->type = m_type;
                    m_pools[iPool].pRecent = pHunk->next;
                    ++m_counts[iPool];
                    m_used[iPool] += rawBytes;
                    return HeaderToUser(pHunk);
                }
                pHunk = pHunk->next;
            }
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
        const i32 rawBytes = pHunk->size;
        const i32 iPool = SelectPool(rawBytes);
        m_pools[iPool].pRecent = Merge(pHunk);
        --m_counts[iPool];
        m_used[iPool] -= rawBytes;
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
