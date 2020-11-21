#pragma once

#include "common/macro.h"
#include "allocator/allocator.h"
#include "containers/sort.hpp"
#include <string.h>
#include <new>

template<typename T>
class Array final
{
private:
    T* m_ptr;
    i32 m_length;
    EAlloc m_allocator;

public:
    bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    EAlloc AllocType() const { return m_allocator; }
    i32 Size() const { return m_length; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_length; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_length; }
    T& front() { ASSERT(InRange(0)); return m_ptr[0]; }
    const T& front() const { ASSERT(InRange(0)); return m_ptr[0]; }
    T& back() { ASSERT(InRange(0)); return m_ptr[m_length - 1]; }
    const T& back() const { ASSERT(InRange(0)); return m_ptr[m_length - 1]; }
    T& operator[](i32 i) { ASSERT(InRange(i)); return m_ptr[i]; }
    const T& operator[](i32 i) const { ASSERT(InRange(i)); return m_ptr[i]; }

    explicit Array(EAlloc allocator = EAlloc_Perm, i32 minSize = 0)
    {
        m_ptr = NULL;
        m_length = 0;
        m_allocator = allocator;
        Reserve(minSize);
    }

    ~Array()
    {
        Reset();
    }

    explicit Array(Array&& other)
    {
        m_ptr = other.m_ptr;
        m_length = other.m_length;
        m_allocator = other.m_allocator;
        other.m_ptr = NULL;
        other.m_length = 0;
    }
    Array& operator=(Array&& other)
    {
        Reset();
        m_ptr = other.m_ptr;
        m_length = other.m_length;
        m_allocator = other.m_allocator;
        other.m_ptr = NULL;
        other.m_length = 0;
        return *this;
    }

    explicit Array(const Array& other)
    {
        m_ptr = NULL;
        m_length = 0;
        m_allocator = other.m_allocator;

        const i32 length = other.m_length;
        Reserve(length);
        m_length = length;
        T *const pim_noalias dst = m_ptr;
        T const *const pim_noalias src = other.m_ptr;
        for (i32 i = 0; i < length; ++i)
        {
            new (dst + i) T(src[i]);
        }
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
        i32 b = --m_length;
        m_ptr[b].~T();
    }

    void Remove(i32 index)
    {
        ASSERT(InRange(index));
        T *const pim_noalias ptr = m_ptr;
        i32 b = --m_length;
        ptr[index].~T();
        memcpy(ptr + index, ptr + b, sizeof(T));
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
        ASSERT(capacity >= 0);
        if (capacity > m_length)
        {
            m_ptr = (T*)pim_realloc(m_allocator, m_ptr, sizeof(T) * capacity);
        }
    }

    void Resize(i32 newLength)
    {
        ASSERT(newLength >= 0);
        Reserve(newLength);
        T *const pim_noalias ptr = m_ptr;
        const i32 oldLength = m_length;
        for (i32 i = oldLength - 1; i >= newLength; --i)
        {
            ptr[i].~T();
        }
        for (i32 i = oldLength - 1; i < newLength; ++i)
        {
            new (ptr + i) T();
        }
        m_length = newLength;
    }

    T& Grow()
    {
        i32 b = m_length++;
        m_ptr = (T*)pim_realloc(m_allocator, m_ptr, sizeof(T) * (b + 1));
        T *const pim_noalias ptr = m_ptr;
        new (ptr + b) T();
        return ptr[b];
    }

    i32 Find(const T& key) const
    {
        const i32 length = m_length;
        T const *const pim_noalias ptr = m_ptr;
        for (i32 i = length - 1; i >= 0; --i)
        {
            if (ptr[i] == key)
            {
                return i;
            }
        }
        return -1;
    }

    bool Contains(const T& key) const { return Find(key) >= 0; }

    bool FindAdd(const T& key)
    {
        if (!Contains(key))
        {
            Grow() = key;
            return true;
        }
        return false;
    }

    bool FindRemove(const T& key)
    {
        i32 index = Find(key);
        if (index >= 0)
        {
            Remove(index);
            return true;
        }
        return false;
    }

    void Sort()
    {
        QuickSort(m_ptr, m_length);
    }
};
