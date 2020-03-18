#pragma once

#include "common/int_types.h"
#include "common/minmax.h"
#include "containers/hash_util.h"
#include "allocator/allocator.h"

template<typename T>
struct Queue
{
    EAlloc m_allocator;
    u32 m_width;
    u32 m_iRead;
    u32 m_iWrite;
    T* m_ptr;

    // ------------------------------------------------------------------------

    void Init(EAlloc allocator = EAlloc_Perm, u32 capacity = 0)
    {
        m_allocator = allocator;
        m_width = 0;
        m_iRead = 0;
        m_iWrite = 0;
        m_ptr = 0;
        if (capacity > 0)
        {
            Reserve(capacity);
        }
    }

    void Reset()
    {
        CAllocator.Free(m_ptr);
        m_ptr = 0;
        m_width = 0;
        m_iRead = 0;
        m_iWrite = 0;
    }

    void Clear()
    {
        m_iWrite = 0;
        m_iRead = 0;
    }

    void Trim()
    {
        FitTo(size());
    }

    // ------------------------------------------------------------------------

    EAlloc GetAllocator() const { return m_allocator; }
    u32 Mask() const { return m_width - 1u; }
    u32 capacity() const { return m_width; }
    u32 size() const { return m_iWrite - m_iRead; }
    bool HasItems() const { return size() != 0u; }
    bool IsEmpty() const { return size() == 0u; }

    // ------------------------------------------------------------------------

    void FitTo(u32 newSize)
    {
        const u32 newWidth = HashUtil::ToPow2(newSize);
        const u32 oldWidth = m_width;
        const u32 length = size();

        ASSERT(newSize >= 0u);
        ASSERT(newWidth >= 0u);
        ASSERT(length <= newWidth);

        if (newWidth == oldWidth)
        {
            return;
        }

        const u32 oldMask = Mask();
        const u32 oldiRead = m_iRead;
        T* newPtr = Allocator::AllocT<T>(GetAllocator(), newWidth);
        T* oldPtr = m_ptr;

        for (u32 i = 0; i < length; ++i)
        {
            const u32 j = (oldiRead + i) & oldMask;
            newPtr[i] = oldPtr[j];
        }

        CAllocator.Free(oldPtr);

        m_ptr = newPtr;
        m_width = newWidth;
        m_iRead = 0;
        m_iWrite = length;
    }

    void Reserve(u32 newSize)
    {
        const u32 width = m_width;
        if (newSize >= width)
        {
            FitTo(Max(newSize, Max(width * 2u, 16u)));
        }
    }

    // ------------------------------------------------------------------------

    u32 GetIndex(u32 i) const
    {
        return (m_iRead + i) & Mask();
    }

    T& operator[](u32 i)
    {
        ASSERT(HasItems());
        return m_ptr[GetIndex(i)];
    }

    const T& operator[](u32 i) const
    {
        ASSERT(HasItems());
        return m_ptr[GetIndex(i)];
    }

    T Pop()
    {
        ASSERT(HasItems());
        const u32 i = m_iRead++ & Mask();
        return m_ptr[i];
    }

    T Peek() const
    {
        ASSERT(HasItems());
        const u32 i = m_iRead & Mask();
        return m_ptr[i];
    }

    bool TryPop(T& dst)
    {
        if (HasItems())
        {
            dst = Pop();
            return true;
        }
        return false;
    }

    bool TryPeek(T& dst)
    {
        if (HasItems())
        {
            dst = Peek();
            return true;
        }
        return false;
    }

    void Push(T value)
    {
        Reserve(size() + 1u);
        const u32 i = m_iWrite++ & Mask();
        m_ptr[i] = value;
    }

    u32 PushSort(T value)
    {
        Push(value);

        const u32 mask = Mask();
        const u32 iRead = m_iRead;
        T* const ptr = m_ptr;

        const u32 back = size() - 1u;
        u32 pos = back;
        for (u32 i = back; i > 0u; --i)
        {
            const u32 rhs = (iRead + i) & mask;
            const u32 lhs = (iRead + i - 1u) & mask;

            if (ptr[rhs] < ptr[lhs])
            {
                T tmp = ptr[lhs];
                ptr[lhs] = ptr[rhs];
                ptr[rhs] = tmp;
                --pos;
            }
            else
            {
                break;
            }
        }
        return pos;
    }

    // ------------------------------------------------------------------------

    struct iterator
    {
        T* const m_ptr;
        const u32 m_mask;
        u32 m_i;

        inline iterator(Queue& queue, bool isBegin)
            : m_ptr(queue.m_ptr),
            m_mask(queue.Mask()),
            m_i(isBegin ? queue.m_iRead : queue.m_iWrite)
        {}

        inline bool operator!=(iterator rhs) const
        {
            return m_i != rhs.m_i;
        }

        inline iterator& operator++()
        {
            ++m_i;
            return *this;
        }

        inline T& operator*()
        {
            return m_ptr[m_i & m_mask];
        }
    };

    inline iterator begin() { return iterator(*this, true); }
    inline iterator end() { return iterator(*this, false); }

    // ------------------------------------------------------------------------

    struct const_iterator
    {
        const T* const m_ptr;
        const u32 m_mask;
        u32 m_i;

        inline const_iterator(const Queue& queue, bool isBegin)
            : m_ptr(queue.m_ptr),
            m_mask(queue.Mask()),
            m_i(isBegin ? queue.m_iRead : queue.m_iWrite)
        {}

        inline bool operator!=(const_iterator rhs) const
        {
            return m_i != rhs.m_i;
        }

        inline const_iterator& operator++()
        {
            ++m_i;
            return *this;
        }

        inline const T& operator*() const
        {
            return m_ptr[m_i & m_mask];
        }
    };

    inline const_iterator begin() const { return const_iterator(*this, true); }
    inline const_iterator end() const { return const_iterator(*this, false); }

    // ------------------------------------------------------------------------
};

template<typename T>
inline i32 Find(const Queue<T> queue, const T& key)
{
    const u32 count = queue.size();
    for (u32 i = 0u; i < count; ++i)
    {
        if (key == queue[i])
        {
            return (i32)i;
        }
    }
    return -1;
}

template<typename T>
inline void Remove(Queue<T>& queue, u32 i)
{
    Queue<T> local = queue;

    const u32 count = local.size();
    const u32 back = count - 1u;
    ASSERT(i < count);

    for (; i < back; ++i)
    {
        local[i] = local[i + 1u];
    }

    queue.Pop();
}

template<typename T>
static Queue<T> CreateQueue(EAlloc allocator = EAlloc_Perm, i32 cap = 0)
{
    ASSERT(cap >= 0);
    Queue<T> queue;
    queue.Init(allocator);
    if (cap > 0)
    {
        queue.Reserve((u32)cap);
    }
    return queue;
}
