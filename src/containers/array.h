#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include <string.h>

template<typename T>
struct Array
{
    AllocType m_allocType;
    T* m_ptr;
    i32 m_length;
    i32 m_capacity;

    // ------------------------------------------------------------------------

    inline void Init(AllocType allocType)
    {
        m_ptr = nullptr;
        m_length = 0;
        m_capacity = 0;
        m_allocType = allocType;
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
            m_ptr = (T*)Allocator::Realloc(m_allocType, m_ptr, sizeof(T) * len);
            m_capacity = len;
        }
    }

    // ------------------------------------------------------------------------

    inline bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    inline i32 Size() const { return m_length; }
    inline i32 Capacity() const { return m_capacity; }
    inline bool IsEmpty() const { return m_length == 0; }
    inline bool IsFull() const { return m_length == m_capacity; }

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
        return { begin(), Size() };
    }
    inline Slice<T> AsSlice()
    {
        return { begin(), Size() };
    }

    inline operator Slice<const T>() const { return AsSlice(); }
    inline operator Slice<T>() { return AsSlice(); }

    inline void Reserve(i32 newCap)
    {
        m_ptr = Allocator::Reserve<T>(m_allocType, m_ptr, m_capacity, newCap);
    }
    inline void ReserveRel(i32 relSize)
    {
        Reserve(m_length + relSize);
    }
    inline void Resize(i32 newLen)
    {
        Reserve(newLen);
        m_length = newLen;
    }
    inline i32 ResizeRel(i32 relSize)
    {
        i32 prevLen = m_length;
        Resize(m_length + relSize);
        return prevLen;
    }
    inline T& Grow()
    {
        const i32 iBack = m_length;
        const i32 newLen = iBack + 1;
        Reserve(newLen);
        m_length = newLen;
        T& item = m_ptr[iBack];
        memset(&item, 0, sizeof(T));
        return item;
    }
    inline void Pop()
    {
        ASSERT(m_length > 0);
        --m_length;
    }
    inline T PopValue()
    {
        T item = back();
        Pop();
        return item;
    }
    inline T PopFront()
    {
        T* ptr = m_ptr;
        const i32 len = m_length--;
        ASSERT(len > 0);
        T item = ptr[0];
        for (i32 i = 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
        return item;
    }
    inline void Remove(i32 i)
    {
        T* ptr = m_ptr;
        const i32 b = --m_length;
        ASSERT((u32)i <= (u32)b);
        ptr[i] = ptr[b];
    }
    inline void ShiftRemove(i32 idx)
    {
        ASSERT((u32)idx < (u32)m_length);
        T* ptr = begin();
        const i32 len = m_length--;
        for (i32 i = idx + 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
    }
    inline void ShiftInsert(i32 idx, const T& value)
    {
        const i32 len = ++m_length;
        ASSERT((u32)idx < (u32)len);
        Reserve(len);
        T* ptr = m_ptr;
        for (i32 i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void ShiftTail(i32 tailSize)
    {
        ASSERT(tailSize >= 0);
        T* ptr = m_ptr;
        const i32 len = m_length;
        const i32 diff = len - tailSize;
        if (diff > 0)
        {
            for (i32 i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_length = tailSize;
        }
    }
    inline void Copy(Slice<const T> other)
    {
        Resize(other.Size());
        memcpy(begin(), other.begin(), sizeof(T) * Size());
    }
    inline void Copy(const Slice<T> other)
    {
        Resize(other.Size());
        memcpy(begin(), other.begin(), sizeof(T) * Size());
    }
    inline void Assume(Array<T>& other)
    {
        Reset();
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
    }
};
