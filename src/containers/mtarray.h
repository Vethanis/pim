#pragma once

#include "containers/array.h"
#include "os/thread.h"
#include "os/atomics.h"

template<typename T>
struct MtArray
{
    mutable OS::RWLock m_lock;
    T* m_ptr;
    mutable OS::RWFlag* m_flags;
    i32 m_innerLength;
    i32 m_length;
    i32 m_capacity;
    AllocType m_allocator;

    static constexpr bool kIsInt = (sizeof(T) == sizeof(i32)) && (alignof(T) == alignof(i32));
    static constexpr bool kIsPtr = (sizeof(T) == sizeof(isize)) && (alignof(T) == alignof(isize));
    static constexpr bool kFlagsOff = kIsInt || kIsPtr;
    static constexpr bool kFlagsOn = !kFlagsOff;

    void Init(AllocType allocator)
    {
        m_lock.Open();
        m_ptr = 0;
        m_flags = 0;
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
        if (kFlagsOn)
        {
            Allocator::Free(m_flags);
            m_flags = 0;
        }
        m_lock.Close();
    }

    i32 size() const
    {
        return Load(m_length);
    }

    i32 capacity() const
    {
        return Load(m_capacity);
    }

    bool InRange(i32 i) const
    {
        return (u32)i < (u32)size();
    }

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

    void Trim()
    {
        if (capacity() > size())
        {
            OS::WriteGuard guard(m_lock);
            const i32 current = capacity();
            const i32 sz = size();
            if (sz < current)
            {
                m_ptr = Allocator::ReallocT<T>(m_allocator, m_ptr, sz);
                if (kFlagsOn)
                {
                    m_flags = Allocator::ReallocT<OS::RWFlag>(m_allocator, m_flags, sz);
                }
                Store(m_capacity, sz);
            }
        }
    }

    void Reserve(i32 minCap)
    {
        ASSERT(minCap >= 0);
        if (minCap > capacity())
        {
            OS::WriteGuard guard(m_lock);
            const i32 current = capacity();
            if (minCap > current)
            {
                const i32 cap = Max(minCap, current * 2, 16);
                m_ptr = Allocator::ReallocT<T>(m_allocator, m_ptr, cap);
                if (kFlagsOn)
                {
                    m_flags = Allocator::ReallocT<OS::RWFlag>(m_allocator, m_flags, cap);
                    for (i32 i = current; i < cap; ++i)
                    {
                        Store(m_flags[i].m_state, 0);
                    }
                }
                Store(m_capacity, cap);
            }
        }
    }

    T _Read(i32 i) const
    {
        if (kFlagsOn)
        {
            OS::ReadFlagGuard guard(m_flags[i]);
            return m_array[i];
        }
        else if (kIsInt)
        {
            T result;
            i32* dst = reinterpret_cast<i32*>(&result);
            const i32& src = reinterpret_cast<const i32&>(m_array[i]);
            *dst = Load(src);
            return result;
        }
        else
        {
            T result;
            isize* dst = reinterpret_cast<isize*>(&result);
            isize& src = reinterpret_cast<const isize&>(m_array[i]);
            *dst = Load(src);
            return result;
        }
    }

    void _Write(i32 i, const T& valueIn)
    {
        if (kFlagsOn)
        {
            OS::WriteFlagGuard guard(m_flags[i]);
            m_array[i] = valueIn;
        }
        else if (kIsInt)
        {
            const i32& src = reinterpret_cast<const i32&>(valueIn);
            i32& dst = reinterpret_cast<i32&>(m_array[i]);
            Store(dst, src);
        }
        else
        {
            const isize& src = reinterpret_cast<const isize&>(valueIn);
            isize& dst = reinterpret_cast<isize&>(m_array[i]);
            Store(dst, src);
        }
    }

    T Read(i32 i) const
    {
        OS::ReadGuard guard(m_lock);
        ASSERT(InRange(i));
        return _Read(i);
    }

    void Write(i32 i, const T& valueIn)
    {
        OS::ReadGuard guard(m_lock);
        ASSERT(InRange(i));
        _Write(i, valueIn);
    }

    bool CmpEx(i32 i, i32& expected, i32 desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed)
    {
        ASSERT(kIsInt);
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        T& rT = m_ptr[i];
        return CmpExStrong((i32&)rT, expected, desired, success, failure);
    }

    bool CmpEx(i32 i, isize& expected, isize desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed)
    {
        ASSERT(kIsPtr);
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        T& rT = m_ptr[i];
        return CmpExStrong((isize&)rT, expected, desired, success, failure);
    }

    i32 PushBack(const T& value)
    {
        const i32 i = Inc(m_innerLength);
        Reserve(i + 1);
        m_lock.LockReader();
        _Write(i, value);
        m_lock.UnlockReader();
        Inc(m_length);
        return i;
    }

    T PopBack()
    {
        const i32 i = Dec(m_length) - 1;
        ASSERT(i >= 0);
        m_lock.LockReader();
        T value = _Read(i);
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
        _Write(i, backVal);
    }

    i32 Find(const T& key, const Equatable<T>& eq) const
    {
        OS::ReadGuard guard(m_lock);
        const T* ptr = m_ptr;
        const i32 ct = size();
        if (kFlagsOn)
        {
            const OS::RWFlag* flags = m_flags;
            for (i32 i = ct - 1; i >= 0; --i)
            {
                OS::ReadFlagGuard g2(flags[i]);
                if (eq(key, ptr[i]))
                {
                    return i;
                }
            }
        }
        else
        {
            for (i32 i = ct - 1; i >= 0; --i)
            {
                if (eq(key, ptr[i]))
                {
                    return i;
                }
            }
        }
        return -1;
    }

    bool Contains(const T& key, const Equatable<T>& eq) const
    {
        return Find(key, eq) != -1;
    }

    bool FindAdd(const T& key, const Equatable<T>& eq)
    {
        i32 i = Find(key, eq);
        if (i == -1)
        {
            PushBack(key);
            return true;
        }
        return false;
    }

    bool FindRemove(const T& key, const Equatable<T>& eq)
    {
        i32 i = Find(key, eq);
        if (i != -1)
        {
            RemoveAt(i);
            return true;
        }
        return false;
    }
};
