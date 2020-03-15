#pragma once

#include "allocator/allocator.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "common/minmax.h"

struct PtrQueue
{
    OS::RWLock m_lock;
    u32 m_iWrite;
    u32 m_iRead;
    u32 m_width;
    isize* m_ptr;
    AllocType m_allocator;

    void Init(AllocType allocator = Alloc_Perm, u32 minCap = 0u)
    {
        m_iWrite = 0;
        m_iRead = 0;
        m_width = 0;
        m_ptr = 0;
        m_lock = {};
        m_lock.Open();
        m_allocator = allocator;
        if (minCap > 0u)
        {
            Reserve(minCap);
        }
    }

    void Reset()
    {
        m_lock.LockWriter();
        Allocator::Free(m_ptr);
        m_ptr = 0;
        m_width = 0;
        m_iWrite = 0;
        m_iRead = 0;
        store_u32(&m_width, 0, MO_Relaxed);
        store_u32(&m_iWrite, 0, MO_Relaxed);
        store_u32(&m_iRead, 0, MO_Relaxed);
        m_lock.Close();
    }

    AllocType GetAllocator() const { return m_allocator; }
    u32 capacity() const { return load_u32(&m_width, MO_Acquire); }
    u32 size() const { return load_u32(&m_iWrite, MO_Acquire) - load_u32(&m_iRead, MO_Acquire); }

    void Clear()
    {
        OS::ReadGuard guard(m_lock);
        store_u32(&m_iWrite, 0, MO_Relaxed);
        store_u32(&m_iRead, 0, MO_Relaxed);
        const u32 width = load_u32(&m_width, MO_Relaxed);
        isize* const ptr = m_ptr;
        for (u32 i = 0; i < width; ++i)
        {
            store_ptr(ptr + i, 0, MO_Relaxed);
        }
    }

    void Reserve(u32 minCap)
    {
        minCap = Max(minCap, 16u);
        const u32 newWidth = ToPow2(minCap);
        if (newWidth > capacity())
        {
            isize* newPtr = Allocator::CallocT<isize>(m_allocator, newWidth);

            m_lock.LockWriter();

            isize* oldPtr = m_ptr;
            const u32 oldWidth = capacity();
            const bool grew = oldWidth < newWidth;

            if (grew)
            {
                const u32 oldMask = oldWidth - 1u;
                const u32 oldTail = load_u32(&m_iRead, MO_Relaxed);
                const u32 oldHead = load_u32(&m_iWrite, MO_Relaxed);
                const u32 len = oldHead - oldTail;

                for (u32 i = 0u; i < len; ++i)
                {
                    const u32 iOld = (oldTail + i) & oldMask;
                    newPtr[i] = oldPtr[iOld];
                }

                m_ptr = newPtr;
                store_u32(&m_width, newWidth, MO_Relaxed);
                store_u32(&m_iRead, 0, MO_Relaxed);
                store_u32(&m_iWrite, len, MO_Relaxed);
            }

            m_lock.UnlockWriter();

            if (grew)
            {
                Allocator::Free(oldPtr);
            }
            else
            {
                Allocator::Free(newPtr);
            }
        }
    }

    void Push(void* pValue)
    {
        ASSERT(pValue);
        const isize iPtr = (isize)pValue;
        while (true)
        {
            Reserve(size() + 1u);
            OS::ReadGuard guard(m_lock);
            const u32 mask = capacity() - 1u;
            isize* const ptr = m_ptr;
            for (u32 i = load_u32(&m_iWrite, MO_Acquire); size() <= mask; ++i)
            {
                i &= mask;
                isize prev = load_ptr(ptr + i, MO_Relaxed);
                if (!prev && cmpex_ptr(ptr + i, &prev, iPtr, MO_Acquire))
                {
                    inc_u32(&m_iWrite, MO_Release);
                    return;
                }
            }
        }
    }

    void* TryPop()
    {
        if (!size())
        {
            return 0;
        }
        OS::ReadGuard guard(m_lock);
        const u32 mask = capacity() - 1u;
        isize* const ptr = m_ptr;
        for (u32 i = load_u32(&m_iRead, MO_Acquire); size() != 0u; ++i)
        {
            i &= mask;
            isize prev = load_ptr(ptr + i, MO_Relaxed);
            if (prev && cmpex_ptr(ptr + i, &prev, 0, MO_Acquire))
            {
                inc_u32(&m_iRead, MO_Release);
                ASSERT(prev);
                return (void*)prev;
            }
        }
        return 0;
    }
};
