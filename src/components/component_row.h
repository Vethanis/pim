#pragma once

#include "common/int_types.h"
#include "common/typeid.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/array.h"
#include "common/chunk_allocator.h"

struct ComponentRow
{
    OS::RWLock m_lock;
    isize* m_ptrs;
    i32 m_length;
    i32 m_capacity;
    ChunkAllocator m_allocator;

    i32 size() const { return Load(m_length, MO_Relaxed); }
    i32 capacity() const { return Load(m_capacity, MO_Relaxed); }
    bool InRange(i32 i) const { return (u32)i < (u32)size(); }

    void Init(i32 stride)
    {
        ASSERT(stride > 0);
        m_lock.Open();
        StorePtr(m_ptrs, (isize*)0);
        Store(m_length, 0);
        Store(m_capacity, 0);
        m_allocator.Init(Alloc_Tlsf, stride);
    }

    void Reset()
    {
        m_lock.LockWriter();
        Store(m_length, 0);
        Store(m_capacity, 0);
        Allocator::Free(ExchangePtr(m_ptrs, (isize*)0));
        m_allocator.Reset();
        m_lock.Close();
    }

    bool Reserve(i32 newSize)
    {
        if (newSize > capacity())
        {
            OS::WriteGuard guard(m_lock);
            const i32 curCap = capacity();
            if (newSize > curCap)
            {
                const i32 newCap = Max(newSize, Max(curCap * 2, 64));
                m_ptrs = Allocator::ReallocT<isize>(Alloc_Tlsf, m_ptrs, newCap);
                memset(m_ptrs + curCap, 0, (newCap - curCap) * sizeof(isize));
                Store(m_capacity, newCap);
            }
        }
        i32 oldSize = size();
        return (oldSize < newSize) && CmpExStrong(m_length, oldSize, newSize, MO_Acquire);
    }

    void* Get(i32 i)
    {
        if (!InRange(i))
        {
            return nullptr;
        }
        OS::ReadGuard guard(m_lock);
        return (void*)Load(m_ptrs[i], MO_Relaxed);
    }

    void* Add(i32 i)
    {
        Reserve(i + 1);
        void* ptr = Get(i);
        if (ptr)
        {
            return ptr;
        }

        ptr = m_allocator.Allocate();
        ASSERT(ptr);

        m_lock.LockReader();
        isize prev = Load(m_ptrs[i], MO_Relaxed);
        if (!prev && CmpExStrong(m_ptrs[i], prev, (isize)ptr, MO_Acquire))
        {
            m_lock.UnlockReader();
            return ptr;
        }
        m_lock.UnlockReader();

        m_allocator.Free(ptr);

        ptr = Get(i);
        ASSERT(ptr);

        return ptr;
    }

    bool Remove(i32 i)
    {
        if (!InRange(i))
        {
            return false;
        }
        m_lock.LockReader();
        isize prev = Load(m_ptrs[i], MO_Relaxed);
        if (prev && CmpExStrong(m_ptrs[i], prev, 0, MO_Acquire))
        {
            m_lock.UnlockReader();
            m_allocator.Free((void*)prev);
            return true;
        }
        m_lock.UnlockReader();
        return false;
    }
};
