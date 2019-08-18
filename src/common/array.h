#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/allocator.h"
#include "common/memory.h"

template<typename T>
struct Array
{
    T* m_ptr;
    i32 m_len;
    i32 m_cap;
    AllocatorType m_alloc;

    inline i32 size() const { return m_len; }
    inline i32 capacity() const { return m_cap; }
    inline usize sizeBytes() const { return (usize)m_len * sizeof(T); }
    inline usize capacityBytes() const { return (usize)m_cap * sizeof(T); }
    inline bool empty() const { return m_len == 0; }
    inline bool full() const { return m_len == m_cap; }
    
    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_len; }
    inline const T& front() const { DebugAssert(!empty()); return m_ptr[0]; }
    inline const T& back() const { DebugAssert(!empty()); return m_ptr[m_len - 1]; }
    inline const T& operator[](i32 i) const { DebugAssert((u32)i < (u32)m_len); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_len; }
    inline T& front() { DebugAssert(!empty()); return m_ptr[0]; }
    inline T& back() { DebugAssert(!empty()); return m_ptr[m_len - 1]; }
    inline T& operator[](i32 i) { DebugAssert((u32)i < (u32)m_len); return m_ptr[i]; }

    inline void init(AllocatorType type = Allocator_Malloc)
    {
        m_ptr = 0;
        m_len = 0;
        m_cap = 0;
        m_alloc = type;
    }
    inline void clear() { m_len = 0; }
    inline void reset()
    {
        Allocator::Free(m_alloc, m_ptr);
        m_ptr = 0;
        m_len = 0;
        m_cap = 0;
    }
    inline void reserve(i32 newCap)
    { 
        i32 cur = m_cap;
        if(newCap > cur)
        {
            i32 next = cur * 2; 
            i32 chosen = newCap > next ? newCap : next; 
            m_ptr = Allocator::Realloc(m_alloc, m_ptr, cur, chosen);
            MemClear(m_ptr + cur, sizeof(T) * (chosen - cur));
            m_cap = chosen;
        }
    }
    inline void resize(i32 newLen) { reserve(newLen); m_len = newLen; }
    inline T& grow()
    {
        reserve(++m_len);
        T& item = back();
        MemClear(&item, sizeof(T));
        return item;
    }
    inline void pop() { DebugAssert(!empty()); --m_len; }
    inline void remove(i32 idx)
    {
        DebugAssert((u32)idx < (u32)m_len);
        m_ptr[idx] = m_ptr[--m_len];
    }
    inline void shiftRemove(i32 idx)
    {
        DebugAssert((u32)idx < (u32)m_len);
        T* ptr = m_ptr;
        const i32 len = m_len--;
        for (i32 i = idx; i + 1 < len; ++i)
        {
            ptr[i] = ptr[i + 1];
        }
    }
    inline void shiftInsert(i32 idx, const T& value)
    {
        const i32 len = ++m_len;
        DebugAssert((u32)idx < (u32)len);
        reserve(len);
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
            if (MemCmp(ptr + i, &value, sizeof(T)) == 0)
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
            if (MemCmp(ptr + i, &value, sizeof(T)) == 0)
            {
                return i;
            }
        }
        return -1;
    }
    inline bool findRemove(const T& value)
    {
        const i32 idx = find(value);
        if (idx != -1)
        {
            remove(idx);
            return true;
        }
        return false;
    }
};
