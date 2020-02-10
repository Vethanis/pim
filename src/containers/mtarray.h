#pragma once

#include "containers/array.h"
#include "os/thread.h"
#include "os/atomics.h"

template<typename T>
struct MtArray
{
    mutable OS::RWLock m_lock;
    T* m_ptr;
    i32 m_innerLength;
    i32 m_length;
    i32 m_capacity;
    AllocType m_allocator;

    void Init(AllocType allocator)
    {
        m_lock.Open();
        m_ptr = 0;
        m_innerLength = 0;
        m_length = 0;
        m_capacity = 0;
        m_allocator = allocator;
    }

    void Reset()
    {
        m_lock.LockWriter();
        Allocator::Free(m_ptr);
        m_ptr = 0;
        m_innerLength = 0;
        m_length = 0;
        m_capacity = 0;
        m_lock.Close();
    }

    AllocType GetAllocator() const { return m_allocator; }
    i32 capacity() const { return Load(m_capacity); }
    i32 size() const { return Load(m_length); }
    bool InRange(i32 i) const { return (u32)i < (u32)size(); }

    void Clear()
    {
        Store(m_innerLength, 0);
        Store(m_length, 0);
    }

    void Resize(i32 newLen)
    {
        Reserve(newLen);
        Store(m_innerLength, newLen);
        Store(m_length, newLen);
    }

    void Reserve(i32 minCap)
    {
        ASSERT(minCap >= 0);
        if (minCap > capacity())
        {
            const i32 cap = Max(minCap, current * 2, 16);
            T* newPtr = Allocator::CallocT<T>(m_allocator, cap);

            m_lock.LockWriter();
            T* oldPtr = LoadPtr(m_ptr);
            const i32 current = capacity();
            const bool grew = cap > current;
            if (grew)
            {
                memcpy(newPtr, oldPtr, sizeof(T) * current);
                StorePtr(m_ptr, newPtr);
                Store(m_capacity, cap);
            }
            m_lock.UnlockWriter();

            Allocator::Free(grew ? oldPtr : newPtr);
        }
    }

    i32 PushBack(T value)
    {
        const i32 i = Inc(m_innerLength);
        Reserve(i + 3);
        m_lock.LockReader();
        Store(m_ptr[i], value);
        m_lock.UnlockReader();
        Inc(m_length);
        return i;
    }

    T PopBack()
    {
        const i32 i = Dec(m_length) - 1;
        ASSERT(i >= 0);
        m_lock.LockReader();
        T value = Load(m_ptr[i]);
        m_lock.UnlockReader();
        const i32 j = Dec(m_innerLength) - 1;
        ASSERT(j >= 0);
        return value;
    }

    void RemoveAt(i32 i)
    {
        ASSERT(InRange(i));
        T backVal = PopBack();
        OS::ReadGuard guard(m_lock);
        Store(m_ptr[i], backVal);
    }

    T IncAt(i32 i)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return Inc(m_ptr[i]);
    }

    T DecAt(i32 i)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return Dec(m_ptr[i]);
    }

    T LoadAt(i32 i) const
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return Load(m_ptr[i]);
    }

    void StoreAt(i32 i, T value) const
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        Store(m_ptr[i], value);
    }

    T ExchangeAt(i32 i, T value)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return Exchange(m_ptr[i], value);
    }

    bool CmpExAt(i32 i, T& expected, T desired)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return CmpExStrong(m_ptr[i], expected, desired);
    }

    T FetchAddAt(i32 i, T add)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return FetchAdd(m_ptr[i], add);
    }

    T FetchSubAt(i32 i, T sub)
    {
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        return FetchSub(m_ptr[i], sub);
    }

    const T* Borrow()
    {
        m_lock.LockReader();
        return LoadPtr(m_ptr);
    }

    void Return(const T* ptr)
    {
        ASSERT(ptr == LoadPtr(m_ptr));
        m_lock.UnlockReader();
    }
};
