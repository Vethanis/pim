#pragma once

#include "common/macro.h"
#include "allocator/allocator.h"
#include "common/nextpow2.h"
#include <string.h>
#include <new>

template<typename T>
class Queue
{
private:
    T* pim_noalias m_ptr;
    u32 m_width;
    u32 m_read;
    u32 m_write;
    EAlloc m_allocator;
public:
    i32 Size() const { return (i32)(m_write - m_read); }
    i32 Capacity() const { return (i32)m_width; }
    EAlloc GetAllocator() const { return m_allocator; }

    explicit Queue(EAlloc allocator = EAlloc_Perm)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
    }
    ~Queue()
    {
        Reset();
    }

    explicit Queue(const Queue& other)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = other.m_allocator;
        Reserve(other.Size());
        for (const T& item : other)
        {
            PushCopy(item);
        }
    }
    Queue& operator=(const Queue& other)
    {
        Reset();
        Reserve(other.Size());
        for (const T& item : other)
        {
            PushCopy(item);
        }
        return *this;
    }

    explicit Queue(Queue&& other)
    {
        EAlloc allocator = other.m_allocator;
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
        other.m_allocator = allocator;
    }
    Queue& operator=(Queue&& other)
    {
        Reset();
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
        other.m_allocator = m_allocator;
        return *this;
    }

    void Clear()
    {
        for (T& item : *this)
        {
            item.~T();
        }
        m_read = 0;
        m_write = 0;
    }

    void Reset()
    {
        Clear();
        pim_free(m_ptr);
        m_ptr = NULL;
        m_width = 0;
    }

    void Reserve(i32 minSize)
    {
        if (minSize <= 0)
        {
            return;
        }
        minSize = minSize > 16 ? minSize : 16;
        const u32 newWidth = NextPow2((u32)minSize);
        const u32 oldWidth = m_width;
        if (newWidth > oldWidth)
        {
            T *const pim_noalias oldPtr = m_ptr;
            T *const pim_noalias newPtr = (T*)pim_malloc(m_allocator, sizeof(T) * newWidth);
            const u32 r = m_read;
            const u32 len = m_write - r;
            if (len)
            {
                const u32 mask = oldWidth - 1u;
                const u32 twist = r & mask;
                const u32 untwist = oldWidth - twist;
                memcpy(newPtr, oldPtr + twist, sizeof(T) * untwist);
                memcpy(newPtr + untwist, oldPtr, twist);
            }
            pim_free(oldPtr);
            m_ptr = newPtr;
            m_width = newWidth;
            m_read = 0;
            m_write = len;
        }
    }

    void PushMove(T&& pim_noalias src)
    {
        Reserve(Size() + 1);
        u32 mask = m_width - 1u;
        u32 w = m_write++ & mask;
        T *const pim_noalias ptr = m_ptr;
        new (ptr + w) T((T&& pim_noalias)src);
    }

    void PushCopy(const T& pim_noalias src)
    {
        Reserve(Size() + 1);
        u32 mask = m_width - 1u;
        u32 w = m_write++ & mask;
        T *const pim_noalias ptr = m_ptr;
        new (ptr + w) T(src);
    }

    bool TryPop(T& pim_noalias dst)
    {
        if (Size())
        {
            u32 mask = m_width - 1u;
            u32 r = m_read++ & mask;
            T *const pim_noalias ptr = m_ptr;
            dst = (T&& pim_noalias)(ptr[r]);
            ptr[r].~T();
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------------------

    class iterator
    {
    private:
        T *const pim_noalias m_ptr;
        const u32 m_mask;
        const u32 m_write;
        u32 m_read;
    public:
        explicit iterator(Queue& queue, u32 index) :
            m_ptr(queue.m_ptr),
            m_mask(queue.m_width - 1u),
            m_write(queue.m_write),
            m_read(index)
        {}
        bool operator!=(const iterator&) const { return m_read != m_write; }
        iterator& operator++() { ++m_read; return *this; }
        T& operator*() { return m_ptr[m_read & m_mask]; }
    };
    iterator begin() { return iterator(*this, m_read); }
    iterator end() { return iterator(*this, m_write); }

    // ------------------------------------------------------------------------

    class const_iterator
    {
    private:
        T const *const pim_noalias m_ptr;
        const u32 m_mask;
        const u32 m_write;
        u32 m_read;
    public:
        explicit const_iterator(const Queue& queue, u32 index) :
            m_ptr(queue.m_ptr),
            m_mask(queue.m_width - 1u),
            m_write(queue.m_write),
            m_read(index)
        {}
        bool operator!=(const const_iterator&) const { return m_read != m_write; }
        const_iterator& operator++() { ++m_read; return *this; }
        const T& operator*() const { return m_ptr[m_read & m_mask]; }
    };
    const_iterator begin() const { return const_iterator(*this, m_read); }
    const_iterator end() const { return const_iterator(*this, m_write); }

};
