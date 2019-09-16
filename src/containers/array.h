#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/allocator.h"
#include <string.h>

template<typename T>
struct Array
{
    Slice<T> m_alloc;
    usize m_len;

    inline usize size() const { return m_len; }
    inline usize capacity() const { return m_alloc.len; }
    inline usize sizeBytes() const { return m_len * sizeof(T); }
    inline usize capacityBytes() const { return m_alloc.len() * sizeof(T); }
    inline bool empty() const { return m_len == 0; }
    inline bool full() const { return m_len == m_alloc.len(); }
    
    inline const T* begin() const { return m_alloc.ptr; }
    inline const T* end() const { return m_alloc.ptr + m_len; }
    inline const T& front() const { DebugAssert(m_len); return m_alloc.ptr[0]; }
    inline const T& back() const { DebugAssert(m_len); return  m_alloc.ptr[m_len - 1]; }
    inline const T& operator[](usize i) const { DebugAssert(i < m_len); return m_alloc.ptr[i]; }

    inline T* begin() { return m_alloc.ptr; }
    inline T* end() { return m_alloc.ptr + m_len; }
    inline T& front() { DebugAssert(m_len); return m_alloc.ptr[0]; }
    inline T& back() { DebugAssert(m_len); return m_alloc.ptr[m_len - 1]; }
    inline T& operator[](usize i) { DebugAssert(i < m_len); return m_alloc.ptr[i]; }

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
    inline void reserve(usize newCap)
    { 
        m_alloc = Allocator::Reserve(m_alloc, newCap);
    }
    inline void reserveRel(isize relSize)
    {
        m_alloc = Allocator::Reserve(m_alloc, m_len + relSize);
    }
    inline void resize(usize newLen)
    {
        reserve(newLen);
        m_len = newLen;
    }
    inline void resizeRel(isize relSize)
    {
        const usize newLen = m_len + relSize;
        reserve(newLen);
        m_len = newLen;
    }
    inline T& grow()
    {
        const usize len = ++m_len;
        reserve(len);
        T& item = m_alloc.ptr[len - 1];
        memset(&item, 0, sizeof(T));
        return item;
    }
    inline void pop()
    {
        DebugAssert(m_len);
        --m_len;
    }
    inline void remove(usize idx)
    {
        DebugAssert(idx < m_len);
        m_alloc.ptr[idx] = m_alloc.ptr[--m_len];
    }
    inline void shiftRemove(usize idx)
    {
        DebugAssert(idx < m_len);
        T* ptr = m_alloc.ptr;
        const isize len = m_len--;
        for (isize i = idx; i + 1 < len; ++i)
        {
            ptr[i] = ptr[i + 1];
        }
    }
    inline void shiftInsert(usize idx, const T& value)
    {
        const isize len = ++m_len;
        DebugAssert(idx < len);
        reserve(len);
        T* ptr = m_alloc.ptr;
        for (isize i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void shiftTail(usize tailSize)
    {
        T* ptr = m_alloc.ptr;
        const isize len = m_len;
        const isize diff = len - tailSize;
        if (diff > 0)
        {
            for (isize i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_len = tailSize;
        }
    }
    inline isize find(const T& value) const
    {
        const T* ptr = m_alloc.ptr;
        const isize len = m_len;
        for (isize i = 0; i < len; ++i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline isize rfind(const T& value) const
    {
        const T* ptr = m_alloc.ptr;
        const isize len = m_len;
        for (isize i = len - 1; i >= 0; --i)
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
        const isize idx = find(value);
        if (idx != -1)
        {
            remove(idx);
            return true;
        }
        return false;
    }
};

template<typename T, usize t_capacity>
struct FixedArray
{
    static constexpr usize Capacity = t_capacity;

    usize m_len;
    T m_ptr[Capacity];

    inline usize size() const { return m_len; }
    inline usize capacity() const { return Capacity; }
    inline usize sizeBytes() const { return m_len * sizeof(T); }
    inline usize capacityBytes() const { return Capacity * sizeof(T); }
    inline bool empty() const { return m_len == 0; }
    inline bool full() const { return m_len == Capacity; }

    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_len; }
    inline const T& front() const { DebugAssert(m_len); return m_ptr[0]; }
    inline const T& back() const { DebugAssert(m_len); return m_ptr[m_len - 1]; }
    inline const T& operator[](usize i) const { DebugAssert(i < m_len); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_len; }
    inline T& front() { DebugAssert(m_len); return m_ptr[0]; }
    inline T& back() { DebugAssert(m_len); return m_ptr[m_len - 1]; }
    inline T& operator[](usize i) { DebugAssert(i < m_len); return m_ptr[i]; }

    inline void init() { m_len = 0; }
    inline void clear() { m_len = 0; }
    inline void reset() { m_len = 0; }
    inline void resize(usize newLen)
    {
        DebugAssert(newLen < Capacity);
        m_len = newLen;
    }
    inline T& grow()
    {
        DebugAssert(m_len != Capacity);
        T& item = m_ptr[m_len++];
        memset(&item, 0, sizeof(T));
        return item;
    }
    inline void pop()
    {
        DebugAssert(m_len);
        --m_len;
    }
    inline void remove(usize idx)
    {
        DebugAssert(idx < m_len);
        m_ptr[idx] = m_ptr[--m_len];
    }
    inline void shiftRemove(isize idx)
    {
        DebugAssert(idx < m_len);
        T* ptr = m_ptr;
        const isize len = m_len--;
        for (isize i = idx; i + 1 < len; ++i)
        {
            ptr[i] = ptr[i + 1];
        }
    }
    inline void shiftInsert(isize idx, const T& value)
    {
        DebugAssert(m_len != Capacity);
        const isize len = ++m_len;
        DebugAssert(idx < len);
        T* ptr = m_ptr;
        for (isize i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void shiftTail(isize tailSize)
    {
        T* ptr = m_ptr;
        const isize len = m_len;
        const isize diff = len - tailSize;
        if (diff > 0)
        {
            for (isize i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_len = tailSize;
        }
    }
    inline isize find(const T& value) const
    {
        const T* ptr = m_ptr;
        const isize len = m_len;
        for (isize i = 0; i < len; ++i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline isize rfind(const T& value) const
    {
        const T* ptr = m_ptr;
        const isize len = m_len;
        for (isize i = len - 1; i >= 0; --i)
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
        const isize idx = find(value);
        if (idx != -1)
        {
            remove(idx);
            return true;
        }
        return false;
    }
};
