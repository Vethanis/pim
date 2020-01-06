#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"

template<typename T>
struct ArrayView
{
    T* m_ptr;
    i32 m_capacity;
    i32 m_size;

    inline i32 capacity() const { return m_capacity; }
    inline i32 size() const { return m_size; }
    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_size; }
    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_size; }

    inline bool InRange(i32 i) const { return (u32)i < (u32)m_size; }
    inline bool IsEmpty() const { return m_size == 0; }
    inline bool IsFull() const { return m_size == m_capacity; }

    inline T& operator[](i32 i)
    {
        ASSERT(InRange(i));
        return m_ptr[i];
    }

    inline const T& operator[](i32 i) const
    {
        ASSERT(InRange(i));
        return m_ptr[i];
    }

    inline T& front()
    {
        ASSERT(InRange(0));
        return m_ptr[0];
    }

    inline const T& front() const
    {
        ASSERT(InRange(0));
        return m_ptr[0];
    }

    inline T& back()
    {
        ASSERT(InRange(m_size - 1));
        return m_ptr[m_size - 1];
    }

    inline const T& back() const
    {
        ASSERT(InRange(m_size - 1));
        return m_ptr[m_size - 1];
    }

    inline Slice<T> AsSlice()
    {
        return { begin(), size() };
    }

    inline Slice<const T> AsSlice() const
    {
        return { begin(), size() };
    }

    inline operator Slice<T>() { return AsSlice(); }
    inline operator Slice<const T>() const { return AsSlice(); }

    inline void Clear()
    {
        m_size = 0;
    }

    inline void Resize(i32 newSize)
    {
        ASSERT((u32)newSize <= (u32)m_capacity);
        m_size = newSize;
    }

    inline void Push(T item)
    {
        ASSERT(!IsFull());
        m_ptr[m_size++] = item;
    }

    inline T Pop()
    {
        ASSERT(!IsEmpty());
        return m_ptr[--m_size];
    }
};
