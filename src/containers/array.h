#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/allocator.h"

template<typename T>
struct Array
{
    Slice<T> m_alloc;
    i32 m_len;

    inline bool in_range(i32 i) const { return (u32)i < (u32)m_len; }
    inline i32 size() const { return m_len; }
    inline i32 capacity() const { return m_alloc.len; }
    inline i32 sizeBytes() const { return m_len * sizeof(T); }
    inline i32 capacityBytes() const { return m_alloc.len() * sizeof(T); }
    inline bool empty() const { return m_len == 0; }
    inline bool full() const { return m_len == m_alloc.len(); }
    
    inline const T* begin() const { return m_alloc.ptr; }
    inline const T* end() const { return m_alloc.ptr + m_len; }
    inline const T& front() const { DebugAssert(in_range(0)); return m_alloc.ptr[0]; }
    inline const T& back() const { DebugAssert(in_range(0)); return  m_alloc.ptr[m_len - 1]; }
    inline const T& operator[](i32 i) const { DebugAssert(in_range(i)); return m_alloc.ptr[i]; }

    inline T* begin() { return m_alloc.ptr; }
    inline T* end() { return m_alloc.ptr + m_len; }
    inline T& front() { DebugAssert(in_range(0)); return m_alloc.ptr[0]; }
    inline T& back() { DebugAssert(in_range(0)); return m_alloc.ptr[m_len - 1]; }
    inline T& operator[](i32 i) { DebugAssert(in_range(i)); return m_alloc.ptr[i]; }

    inline void init()
    {
        m_alloc = { 0, 0 };
        m_len = 0;
    }
    inline void clear()
    {
        m_len = 0;
    }
    inline void reset()
    {
        Allocator::Free(m_alloc);
        m_len = 0;
    }
    inline void reserve(i32 newCap)
    {
        Allocator::Reserve(m_alloc, newCap);
    }
    inline void reserveRel(i32 relSize)
    {
        reserve(m_len + relSize);
    }
    inline void resize(i32 newLen)
    {
        reserve(newLen);
        m_len = newLen;
    }
    inline i32 resizeRel(i32 relSize)
    {
        i32 prevLen = m_len;
        resize(m_len + relSize);
        return prevLen;
    }
    inline T& grow()
    {
        const i32 len = ++m_len;
        reserve(len);
        return m_alloc.ptr[len - 1];
    }
    inline void pop()
    {
        DebugAssert(m_len > 0);
        --m_len;
    }
    inline T popValue()
    {
        T item = back();
        pop();
        return item;
    }
    inline T popFront()
    {
        T* ptr = m_alloc.ptr;
        const i32 len = m_len--;
        DebugAssert(len > 0);
        T item = ptr[0];
        for (i32 i = 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
        return item;
    }
    inline void remove(i32 i)
    {
        T* ptr = m_alloc.ptr;
        const i32 b = --m_len;
        DebugAssert((u32)i <= (u32)b);
        ptr[i] = ptr[b];
    }
    inline void shiftRemove(i32 idx)
    {
        DebugAssert((u32)idx < (u32)m_len);
        T* ptr = begin();
        const i32 len = m_len--;
        for (i32 i = idx + 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
    }
    inline void shiftInsert(i32 idx, const T& value)
    {
        const i32 len = ++m_len;
        DebugAssert((u32)idx < (u32)len);
        reserve(len);
        T* ptr = m_alloc.ptr;
        for (i32 i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void shiftTail(i32 tailSize)
    {
        DebugAssert(tailSize >= 0);
        T* ptr = m_alloc.ptr;
        const i32 len = m_len;
        const i32 diff = len - tailSize;
        if (diff > 0)
        {
            for (i32 i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_len = tailSize;
        }
    }
    inline i32 find(const T& value) const
    {
        const T* ptr = m_alloc.ptr;
        const i32 len = m_len;
        for (i32 i = 0; i < len; ++i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline i32 rfind(const T& value) const
    {
        const T* ptr = m_alloc.ptr;
        const i32 len = m_len;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline bool findRemove(const T& value)
    {
        const i32 idx = rfind(value);
        if (idx != -1)
        {
            remove(idx);
            return true;
        }
        return false;
    }
    inline bool uniquePush(const T& value)
    {
        const i32 idx = rfind(value);
        if (idx == -1)
        {
            grow() = value;
            return true;
        }
        return false;
    }
    inline void copy(const Array<T>& srcArr)
    {
        resize(srcArr.size());
        T* dst = begin();
        const T* src = srcArr.begin();
        const i32 len = m_len;
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = src[i];
        }
    }
    inline void assume(Array<T>& srcArr)
    {
        reset();
        m_alloc = srcArr.m_alloc;
        m_len = srcArr.m_len;
        srcArr.m_alloc = { 0, 0 };
        srcArr.m_len = 0;
    }
};

template<typename T, i32 t_capacity>
struct FixedArray
{
    static constexpr i32 Capacity = t_capacity;

    i32 m_len;
    T m_ptr[Capacity];

    inline i32 size() const { return m_len; }
    inline i32 capacity() const { return Capacity; }
    inline i32 sizeBytes() const { return m_len * sizeof(T); }
    inline i32 capacityBytes() const { return Capacity * sizeof(T); }
    inline bool empty() const { return m_len == 0; }
    inline bool full() const { return m_len == Capacity; }

    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_len; }
    inline const T& front() const { DebugAssert(m_len); return m_ptr[0]; }
    inline const T& back() const { DebugAssert(m_len); return m_ptr[m_len - 1]; }
    inline const T& operator[](i32 i) const { DebugAssert(i < m_len); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_len; }
    inline T& front() { DebugAssert(m_len); return m_ptr[0]; }
    inline T& back() { DebugAssert(m_len); return m_ptr[m_len - 1]; }
    inline T& operator[](i32 i) { DebugAssert(i < m_len); return m_ptr[i]; }

    inline void init() { m_len = 0; }
    inline void clear() { m_len = 0; }
    inline void reset() { m_len = 0; }
    inline void resize(i32 newLen)
    {
        DebugAssert(newLen < Capacity);
        m_len = newLen;
    }
    inline T& grow()
    {
        DebugAssert(m_len < Capacity);
        return  m_ptr[m_len++];
    }
    inline void pop()
    {
        DebugAssert(m_len);
        --m_len;
    }
    inline T popFront()
    {
        T* ptr = begin();
        const i32 len = m_len--;
        DebugAssert(len > 0);
        T item = ptr[0];
        for (i32 i = 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
        return item;
    }
    inline void remove(i32 idx)
    {
        DebugAssert((u32)idx < (u32)m_len);
        m_ptr[idx] = m_ptr[--m_len];
    }
    inline void shiftRemove(i32 idx)
    {
        DebugAssert((u32)idx < (u32)m_len);
        T* ptr = begin();
        const i32 len = m_len--;
        for (i32 i = idx + 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
    }
    inline void shiftInsert(i32 idx, const T& value)
    {
        DebugAssert(m_len < Capacity);
        const i32 len = ++m_len;
        DebugAssert(idx < len);
        T* ptr = m_ptr;
        for (i32 i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void shiftTail(i32 tailSize)
    {
        T* ptr = m_ptr;
        const i32 len = m_len;
        const i32 diff = len - tailSize;
        if (diff > 0)
        {
            for (i32 i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_len = tailSize;
        }
    }
    inline i32 find(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_len;
        for (i32 i = 0; i < len; ++i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline i32 rfind(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_len;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline bool findRemove(const T& value)
    {
        const i32 idx = rfind(value);
        if (idx != -1)
        {
            remove(idx);
            return true;
        }
        return false;
    }
    inline bool uniquePush(const T& value)
    {
        const i32 idx = rfind(value);
        if (idx == -1)
        {
            grow() = value;
            return true;
        }
        return false;
    }
};
