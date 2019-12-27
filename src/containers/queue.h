#pragma once

#include "common/int_types.h"
#include "common/minmax.h"
#include "common/comparator.h"
#include "allocator/allocator.h"

template<typename T>
struct Queue
{
    AllocType m_allocator;
    i32 m_width;
    i32 m_head;
    i32 m_tail;
    T* m_ptr;

    // ------------------------------------------------------------------------

    inline void Init(AllocType allocator)
    {
        m_allocator = allocator;
        m_width = 0;
        m_head = 0;
        m_tail = 0;
        m_ptr = 0;
    }
    inline void Reset()
    {
        Allocator::Free(m_ptr);
        m_ptr = 0;
        m_width = 0;
        m_head = 0;
        m_tail = 0;
    }
    inline void Clear()
    {
        m_tail = 0;
        m_head = 0;
    }
    inline void Trim()
    {
        FitTo(Size());
    }

    // ------------------------------------------------------------------------

    inline AllocType GetAllocType() const { return m_allocator; }
    inline i32 Capacity() const { return m_width; }
    inline i32 Mask() const { return m_width - 1; }
    inline i32 Size() const { return m_tail - m_head; }
    inline i32 Head() const { return m_head & (m_width - 1); }
    inline i32 Tail() const { return m_tail & (m_width - 1); }
    inline bool HasItems() const { return Size() > 0; }
    inline bool IsEmpty() const { return Size() == 0; }

    // ------------------------------------------------------------------------

    inline static i32 NextPow2(i32 x)
    {
        i32 y = x > 0 ? 1 : 0;
        while (y < x)
        {
            y *= 2;
        }
        return y;
    }

    void FitTo(i32 newSize)
    {
        ASSERT(newSize >= 0);
        const i32 minWidth = NextPow2(newSize);
        const i32 width = m_width;
        if (minWidth != width)
        {
            T* newPtr = Allocator::AllocT<T>(GetAllocType(), minWidth);
            T* oldPtr = m_ptr;

            const i32 mask = width - 1;
            const i32 head = m_head;
            const i32 length = m_tail - head;
            for (i32 i = 0; i < length; ++i)
            {
                newPtr[i] = oldPtr[(head + i) & mask];
            }

            Allocator::Free(oldPtr);
            m_ptr = newPtr;
            m_width = minWidth;
            m_head = 0;
            m_tail = length;
        }
    }

    inline void Reserve(i32 newSize)
    {
        ASSERT(newSize >= 0);
        const i32 width = m_width;
        if (newSize > width)
        {
            const i32 nextSize = Max(newSize, Max(width * 2, 16));
            FitTo(nextSize);
        }
    }

    // ------------------------------------------------------------------------

    inline i32 GetIndex(i32 i) const
    {
        const i32 head = m_head;
        const i32 mask = m_width - 1;
        const i32 index = (head + i) & mask;
        return index;
    }

    inline T& operator[](i32 i)
    {
        ASSERT(HasItems());
        return m_ptr[GetIndex(i)];
    }

    inline const T& operator[](i32 i) const
    {
        ASSERT(HasItems());
        return m_ptr[GetIndex(i)];
    }

    inline i32 PopIdx()
    {
        const i32 head = m_head++;
        const i32 mask = m_width - 1;
        const i32 index = head & mask;
        ASSERT((m_tail - head) > 0);
        return index;
    }

    inline i32 PushIdx()
    {
        Reserve(Size() + 1);
        const i32 tail = m_tail++;
        const i32 mask = m_width - 1;
        const i32 index = tail & mask;
        return index;
    }

    inline T Pop()
    {
        return m_ptr[PopIdx()];
    }

    inline void Push(T value)
    {
        m_ptr[PushIdx()] = value;
    }

    inline void Push(T value, Comparator<T> cmp)
    {
        const i32 back = m_tail - m_head;
        Reserve(back + 1);
        const i32 mask = m_width - 1;
        const i32 head = m_head;
        T* const ptr = m_ptr;
        {
            const i32 tail = m_tail++;
            const i32 index = tail & mask;
            ptr[index] = value;
        }
        for (i32 i = back; i > 0; --i)
        {
            const i32 rhs = (head + i) & mask;
            const i32 lhs = (head + i - 1) & mask;
            if (cmp.Compare(ptr[lhs], ptr[rhs]) > 0)
            {
                T tmp = ptr[lhs];
                ptr[lhs] = ptr[rhs];
                ptr[rhs] = tmp;
            }
        }
    }

    void Sort(i32 start, i32 end, Comparator<T> cmp)
    {
        if ((end - start) < 2)
        {
            return;
        }

        i32 i = start;
        {
            const i32 mid = (start + end) >> 1;
            const i32 mask = m_width - 1;
            const i32 head = m_head;
            T* const items = m_ptr;
            const T pivot = items[(head + mid) & mask];

            i32 j = end - 1;
            while (true)
            {
                while ((i < j) &&
                    (cmp.Compare(items[(head + i) & mask], pivot) < 0))
                {
                    ++i;
                }
                while ((j > i) &&
                    (cmp.Compare(items[(head + j) & mask], pivot) > 0))
                {
                    --j;
                }

                if (i >= j)
                {
                    break;
                }

                {
                    const i32 i2 = (head + i) & mask;
                    const i32 j2 = (head + j) & mask;
                    T tmp = items[i2];
                    items[i2] = items[j2];
                    items[j2] = tmp;
                }

                ++i;
                --j;
            }
        }
        Sort(start, i, cmp);
        Sort(i, end, cmp);
    }

    inline void Sort(Comparator<T> cmp)
    {
        Sort(0, Size(), cmp);
    }
};
