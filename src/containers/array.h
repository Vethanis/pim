#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include <string.h>

template<typename T>
struct Array
{
    i32 m_allocator : 4;
    i32 m_capacity : 30;
    i32 m_length : 30;
    T* m_ptr;

    // ------------------------------------------------------------------------

    inline void Init(AllocType allocType)
    {
        m_ptr = nullptr;
        m_length = 0;
        m_capacity = 0;
        m_allocator = allocType;
    }

    inline void Reset()
    {
        Allocator::Free(m_ptr);
        m_ptr = nullptr;
        m_capacity = 0;
        m_length = 0;
    }

    inline void Clear()
    {
        m_length = 0;
    }

    inline void Trim()
    {
        const i32 len = m_length;
        if (m_capacity > len)
        {
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, len);
            m_capacity = len;
        }
    }

    // ------------------------------------------------------------------------

    inline bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    inline AllocType GetAllocator() const { return (AllocType)m_allocator; }
    inline bool IsEmpty() const { return m_length == 0; }
    inline bool IsFull() const { return m_length == m_capacity; }

    inline i32 capacity() const { return m_capacity; }
    inline i32 size() const { return m_length; }
    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_length; }
    inline const T& front() const { ASSERT(InRange(0)); return m_ptr[0]; }
    inline const T& back() const { ASSERT(InRange(0)); return  m_ptr[m_length - 1]; }
    inline const T& operator[](i32 i) const { ASSERT(InRange(i)); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_length; }
    inline T& front() { ASSERT(InRange(0)); return m_ptr[0]; }
    inline T& back() { ASSERT(InRange(0)); return m_ptr[m_length - 1]; }
    inline T& operator[](i32 i) { ASSERT(InRange(i)); return m_ptr[i]; }

    // ------------------------------------------------------------------------

    inline Slice<const T> AsSlice() const
    {
        return { m_ptr, m_length };
    }
    inline Slice<T> AsSlice()
    {
        return { m_ptr, m_length };
    }

    inline operator Slice<const T>() const { return AsSlice(); }
    inline operator Slice<T>() { return AsSlice(); }

    inline void Reserve(i32 newCap)
    {
        const i32 current = m_capacity;
        if (newCap > current)
        {
            i32 next = Max(newCap, Max(current * 2, 16));
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, next);
            m_capacity = next;
        }
    }
    inline void ReserveRel(i32 relSize)
    {
        Reserve(m_length + relSize);
    }
    inline void Resize(i32 newLen)
    {
        ASSERT(newLen >= 0);
        Reserve(newLen);
        m_length = newLen;
    }
    inline i32 ResizeRel(i32 relSize)
    {
        const i32 prevLen = m_length;
        Resize(prevLen + relSize);
        return prevLen;
    }
    inline T& Grow()
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        T& item = m_ptr[iBack];
        memset(&item, 0, sizeof(T));
        return item;
    }
    inline void Pop()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
    }
    inline void PushBack(T item)
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        m_ptr[iBack] = item;
    }
    inline T PopBack()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        return m_ptr[iBack];
    }
    inline T PopFront()
    {
        const T item = front();
        ShiftRemove(0);
        return item;
    }
    inline void PushFront(T item)
    {
        ShiftInsert(0, item);
    }
    inline void Remove(i32 idx)
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        ASSERT((u32)idx <= (u32)iBack);

        T* const ptr = m_ptr;
        ptr[idx] = ptr[iBack];
    }
    inline void ShiftRemove(i32 idx)
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        ASSERT((u32)idx <= (u32)iBack);

        T* const ptr = m_ptr;
        for (i32 i = idx; i < iBack; ++i)
        {
            ptr[i] = ptr[i + 1];
        }
    }
    inline void ShiftInsert(i32 idx, const T& value)
    {
        const i32 length = m_length++;
        ASSERT(idx >= 0);
        ASSERT((u32)idx <= (u32)length);

        Reserve(length + 1);
        T* const ptr = m_ptr;
        for (i32 i = length; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
};

template<typename T>
inline void Copy(Array<T>& dst, Slice<const T> src)
{
    const i32 length = src.size();
    const i32 bytes = sizeof(T) * length;
    dst.Resize(length);
    memcpy(dst.begin(), src.begin(), bytes);
}

template<typename T>
inline void Copy(Array<T>& dst, Slice<T> src)
{
    const i32 length = src.size();
    const i32 bytes = sizeof(T) * length;
    dst.Resize(length);
    memcpy(dst.begin(), src.begin(), bytes);
}

template<typename T>
inline void Move(Array<T>& dst, Array<T>& src)
{
    dst.Reset();
    memcpy(&dst, &src, sizeof(Array<T>));
    memset(&src, 0, sizeof(Array<T>));
}
