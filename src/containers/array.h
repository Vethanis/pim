#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"
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

    void Init(AllocType allocType)
    {
        m_ptr = nullptr;
        m_length = 0;
        m_capacity = 0;
        m_allocator = allocType;
    }

    void Init(AllocType allocator, i32 cap)
    {
        Init(allocator);
        Reserve(cap);
    }

    void Reset()
    {
        Allocator::Free(m_ptr);
        m_ptr = nullptr;
        m_capacity = 0;
        m_length = 0;
    }

    void Clear()
    {
        m_length = 0;
    }

    void Trim()
    {
        const i32 len = m_length;
        if (m_capacity > len)
        {
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, len);
            m_capacity = len;
        }
    }

    // ------------------------------------------------------------------------

    bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    AllocType GetAllocator() const { return (AllocType)m_allocator; }
    bool IsEmpty() const { return m_length == 0; }
    bool IsFull() const { return m_length == m_capacity; }

    i32 capacity() const { return m_capacity; }
    i32 size() const { return m_length; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_length; }
    const T& front() const { ASSERT(InRange(0)); return m_ptr[0]; }
    const T& back() const { ASSERT(InRange(0)); return  m_ptr[m_length - 1]; }
    const T& operator[](i32 i) const { ASSERT(InRange(i)); return m_ptr[i]; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_length; }
    T& front() { ASSERT(InRange(0)); return m_ptr[0]; }
    T& back() { ASSERT(InRange(0)); return m_ptr[m_length - 1]; }
    T& operator[](i32 i) { ASSERT(InRange(i)); return m_ptr[i]; }

    // ------------------------------------------------------------------------

    Slice<const T> AsSlice() const
    {
        return { m_ptr, m_length };
    }

    Slice<T> AsSlice()
    {
        return { m_ptr, m_length };
    }

    operator Slice<const T>() const { return AsSlice(); }
    operator Slice<T>() { return AsSlice(); }

    void Reserve(i32 newCap)
    {
        const i32 current = m_capacity;
        if (newCap > current)
        {
            i32 next = Max(newCap, Max(current * 2, 16));
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, next);
            m_capacity = next;
        }
    }

    void ReserveRel(i32 relSize)
    {
        Reserve(m_length + relSize);
    }

    void Resize(i32 newLen)
    {
        ASSERT(newLen >= 0);
        Reserve(newLen);
        m_length = newLen;
    }

    i32 ResizeRel(i32 relSize)
    {
        const i32 prevLen = m_length;
        Resize(prevLen + relSize);
        return prevLen;
    }

    T& Grow()
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        T& item = m_ptr[iBack];
        memset(&item, 0, sizeof(T));
        return item;
    }

    void Pop()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
    }

    i32 PushBack(T item)
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        m_ptr[iBack] = item;
        return iBack;
    }

    T PopBack()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        return m_ptr[iBack];
    }

    T PopFront()
    {
        const T item = front();
        ShiftRemove(0);
        return item;
    }

    void PushFront(T item)
    {
        ShiftInsert(0, item);
    }

    void Remove(i32 idx)
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        ASSERT((u32)idx <= (u32)iBack);

        T* const ptr = m_ptr;
        ptr[idx] = ptr[iBack];
    }

    void ShiftRemove(i32 idx)
    {
        const i32 oldBack = --m_length;
        const i32 shifts = oldBack - idx;
        ASSERT(oldBack >= 0);
        ASSERT(shifts >= 0);
        T* const ptr = m_ptr;
        memmove(ptr + idx, ptr + idx + 1, sizeof(T) * shifts);
    }

    void ShiftInsert(i32 idx, const T& value)
    {
        const i32 newBack = m_length++;
        const i32 shifts = newBack - idx;
        ASSERT(idx >= 0);
        ASSERT(shifts >= 0);
        Reserve(newBack + 1);
        T* const ptr = m_ptr;
        memmove(ptr + idx + 1, ptr + idx, sizeof(T) * shifts);
        ptr[idx] = value;
    }

    void PushBytes(const void* src, i32 count)
    {
        ASSERT(sizeof(T) == 1);
        ASSERT(src);
        ASSERT(count >= 0);
        const i32 iDst = ResizeRel(count);
        void* dst = m_ptr + iDst;
        memcpy(dst, src, count);
    }

    i32 Find(T key) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_length;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (ptr[i] == key)
            {
                return i;
            }
        }
        return -1;
    }

    bool Contains(T key) const
    {
        return Find(key) != -1;
    }

    bool FindAdd(T value)
    {
        if (!Contains(value))
        {
            PushBack(value);
            return true;
        }
        return false;
    }

    bool FindRemove(T value)
    {
        i32 i = Find(value);
        if (i != -1)
        {
            Remove(i);
            return true;
        }
        return false;
    }
};

template<typename T, typename C>
static void Copy(Array<T>& dst, const C& src)
{
    const i32 length = src.size();
    const i32 bytes = sizeof(T) * length;
    dst.Resize(length);
    memcpy(dst.begin(), src.begin(), bytes);
}

template<typename T>
static void Move(Array<T>& dst, Array<T>& src)
{
    dst.Reset();
    memcpy(&dst, &src, sizeof(Array<T>));
    memset(&src, 0, sizeof(Array<T>));
}

template<typename T>
static Array<T> CreateArray(AllocType allocator, i32 cap)
{
    Array<T> arr;
    arr.Init(allocator, cap);
    return arr;
}
