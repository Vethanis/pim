#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/array.h"

struct HeapItem
{
    i32 offset;
    i32 size;
};

struct Heap
{
    Array<i32> m_sizes;
    Array<i32> m_offsets;
    i32 m_size;
    i32 m_allocated;

    void Init(AllocType allocator, i32 size)
    {
        m_sizes.Init(allocator);
        m_offsets.Init(allocator);
        m_sizes.Grow() = size;
        m_offsets.Grow() = 0;
        m_size = size;
    }
    void Reset()
    {
        m_sizes.Reset();
        m_offsets.Reset();
    }
    void Clear()
    {
        m_sizes.Clear();
        m_offsets.Clear();
        m_sizes.Grow() = m_size;
        m_offsets.Grow() = 0;
    }

    i32 Size() const { return m_size; }
    i32 SizeAllocated() const { return m_allocated; }
    // Usable amount is less due to fragmentation
    i32 SizeFree() const { return m_size - m_allocated; }

    i32 Find(i32 desiredSize) const
    {
        constexpr i32 MaxSize = 0x7fffffff;
        ASSERT(desiredSize < MaxSize);

        const i32 count = m_sizes.Size();
        const i32* sizes = m_sizes.begin();
        i32 j = -1;
        i32 size = MaxSize;
        for (i32 i = 0; i < count; ++i)
        {
            const i32 iSize = sizes[i];
            if (iSize >= desiredSize)
            {
                if (iSize < size)
                {
                    size = iSize;
                    j = i;
                }
            }
        }
        return j;
    }

    i32 AllocAt(i32 index, i32 desiredSize)
    {
        i32 size = m_sizes[index];
        i32 offset = m_offsets[index];
        m_sizes.Remove(index);
        m_offsets.Remove(index);
        if (size > desiredSize)
        {
            i32 remSize = size - desiredSize;
            i32 remOffset = offset + desiredSize;
            size = desiredSize;
            m_sizes.Grow() = remSize;
            m_offsets.Grow() = remOffset;
        }
        ASSERT(offset >= 0);
        ASSERT(size == desiredSize);
        return offset;
    }

    HeapItem Alloc(i32 desiredSize)
    {
        ASSERT(desiredSize > 0);
        const i32 i = Find(desiredSize);
        if (i == -1)
        {
            return { -1, 0 };
        }
        const i32 offset = AllocAt(i, desiredSize);
        m_allocated += desiredSize;
        ASSERT(m_allocated <= m_size);
        HeapItem item;
        item.offset = offset;
        item.size = desiredSize;
        return item;
    }

    void Free(HeapItem item)
    {
        i32 offset = item.offset;
        i32 size = item.size;
        i32 end = size + offset;

        ASSERT(size > 0);
        ASSERT(offset >= 0);
        ASSERT(end > 0);

        m_allocated -= size;
        ASSERT(m_allocated >= 0);

        i32* sizes = m_sizes.begin();
        i32* offsets = m_offsets.begin();
        i32 count = m_sizes.Size();

        for (i32 i = count - 1; i >= 0; --i)
        {
            const i32 iSize = sizes[i];
            const i32 iOffset = offsets[i];
            const i32 iEnd = iSize + iOffset;
            ASSERT(iEnd > 0);
            ASSERT(!Overlaps(offset, end, iOffset, iEnd));

            if (Adjacent(offset, end, iOffset, iEnd))
            {
                offset = Min(offset, iOffset);
                size += iSize;
                end = offset + size;

                --count;
                sizes[i] = sizes[count];
                offsets[i] = offsets[count];
                i = count;
            }
        }

        m_sizes.Resize(count + 1);
        m_offsets.Resize(count + 1);
        m_sizes[count] = size;
        m_offsets[count] = offset;
    }
};
