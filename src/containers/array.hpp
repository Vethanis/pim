#pragma once

#include "common/macro.h"
#include "allocator/allocator.h"
#include <string.h>

template<typename T>
class Array final
{
private:
    T* m_ptr;
    i32 m_length;
    EAlloc m_allocator;
public:

    i32 size() const { return m_length; }
    EAlloc AllocType() const { return m_allocator; }
    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_length; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_length; }
    T& front() { ASSERT(m_length > 0); return m_ptr[0]; }
    const T& front() const { ASSERT(m_length > 0); return m_ptr[0]; }
    T& back() { ASSERT(m_length > 0); return m_ptr[m_length - 1]; }
    const T& back() const { ASSERT(m_length > 0); return m_ptr[m_length - 1]; }
    T& operator[](i32 i) { ASSERT((u32)i < (u32)m_length); return m_ptr[i]; }
    const T& operator[](i32 i) const { ASSERT((u32)i < (u32)m_length); return m_ptr[i]; }

    explicit Array()
    {
        m_ptr = NULL;
        m_length = 0;
        m_allocator = EAlloc_Perm;
    }

    ~Array()
    {
        Reset();
    }

    explicit Array(i32 capacity)
    {
        m_ptr = NULL;
        m_length = 0;
        m_allocator = EAlloc_Perm;
        Reserve(capacity);
    }

    explicit Array(EAlloc allocator)
    {
        m_ptr = NULL;
        m_length = 0;
        m_allocator = allocator;
    }

    explicit Array(Array&& other)
    {
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
    }

    explicit Array(const Array& other)
    {
        const i32 length = other.size();
        Reserve(length);
        m_length = length;
        T *const pim_noalias dst = m_ptr;
        T const *const pim_noalias src = other.m_ptr;
        for (i32 i = 0; i < length; ++i)
        {
            new (dst + i) T(src[i]);
        }
    }

    Array& operator=(Array&& other)
    {
        Reset();
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
        return *this;
    }

    Array& operator=(const Array& other)
    {
        const i32 length = other.size();
        Clear();
        Reserve(length);
        m_length = length;
        T *const pim_noalias dst = m_ptr;
        T const *const pim_noalias src = other.m_ptr;
        for (i32 i = 0; i < length; ++i)
        {
            new (dst + i) T(src[i]);
        }
        return *this;
    }

    void Pop()
    {
        ASSERT(m_length > 0);
        i32 index = --m_length;
        m_ptr[index].~T();
    }

    void Clear()
    {
        T *const pim_noalias ptr = m_ptr;
        const i32 length = m_length;
        for (i32 i = length - 1; i >= 0; --i)
        {
            ptr[i].~T();
        }
        m_length = 0;
    }

    void Reset()
    {
        Clear();
        pim_free(m_ptr);
        m_ptr = NULL;
    }

    void Reserve(i32 capacity)
    {
        if (capacity > m_length)
        {
            m_ptr = pim_realloc(m_allocator, m_ptr, sizeof(T) * capacity);
        }
    }

    T& Grow()
    {
        const i32 index = m_length;
        Reserve(index + 1);
        m_length = index + 1;
        T *const pim_noalias ptr = m_ptr;
        new (ptr + index)T();
        return ptr[index];
    }
};
