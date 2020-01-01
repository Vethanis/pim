#pragma once

#include "common/int_types.h"
#include "common/minmax.h"
#include "common/comparator.h"
#include "containers/hash_util.h"
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

    void FitTo(i32 newSize)
    {
        const i32 newWidth = HashUtil::ToPow2(newSize);
        const i32 oldWidth = m_width;
        const i32 oldMask = oldWidth - 1;
        const i32 oldHead = m_head;
        const i32 length = m_tail - oldHead;
        const AllocType allocator = m_allocator;

        ASSERT(newSize >= 0);
        ASSERT(newWidth >= 0);
        ASSERT((u32)length <= (u32)oldWidth);
        ASSERT((u32)length <= (u32)newWidth);

        // no resize
        if (newWidth == oldWidth)
        {
            return;
        }

        // no wraparound
        const i32 rotation = oldHead & oldMask;
        if ((rotation + length) <= oldWidth)
        {
            if (rotation != 0)
            {
                T* const ptr = m_ptr;
                for (i32 i = 0; i < length; ++i)
                {
                    ptr[i] = ptr[i + rotation];
                }
            }

            m_ptr = Allocator::ReallocT<T>(allocator, m_ptr, newWidth);
            m_width = newWidth;
            m_head = 0;
            m_tail = length;

            return;
        }

        // >= 2x growth
        if (newWidth >= (oldWidth * 2))
        {
            m_ptr = Allocator::ReallocT<T>(allocator, m_ptr, newWidth);

            // un-rotate into new side
            T* const oldSide = m_ptr;
            T* const newSide = m_ptr + oldWidth;
            for (i32 i = 0; i < length; ++i)
            {
                const i32 j = (oldHead + i) & oldMask;
                newSide[i] = oldSide[j];
            }

            // head is now at midpoint of resized allocation
            m_head = oldWidth;
            m_tail = oldWidth + length;
            m_width = newWidth;

            return;
        }

        // < 2x growth
        {
            // create a new allocation
            T* newPtr = Allocator::AllocT<T>(allocator, newWidth);
            T* oldPtr = m_ptr;

            // un-rotate into new allocation
            for (i32 i = 0; i < length; ++i)
            {
                const i32 j = (oldHead + i) & oldMask;
                newPtr[i] = oldPtr[j];
            }

            // free old allocation
            Allocator::Free(oldPtr);

            m_ptr = newPtr;
            m_width = newWidth;
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
            FitTo(Max(newSize, Max(width * 2, 16)));
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

    inline T Pop()
    {
        const i32 head = m_head++;
        const i32 mask = m_width - 1;
        const i32 index = head & mask;
        ASSERT((m_tail - head) > 0);
        return m_ptr[index];
    }

    inline void Push(T value)
    {
        Reserve(Size() + 1);
        const i32 tail = m_tail++;
        const i32 mask = m_width - 1;
        const i32 index = tail & mask;
        m_ptr[index] = value;
    }

    void Push(T value, const Comparable<T> cmp)
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

            if (cmp(ptr[lhs], ptr[rhs]) > 0)
            {
                T tmp = ptr[lhs];
                ptr[lhs] = ptr[rhs];
                ptr[rhs] = tmp;
            }
            else
            {
                break;
            }
        }
    }
};
