#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "common/find.h"
#include "common/sort.h"
#include "containers/array.h"

struct HeapItem
{
    i32 offset;
    i32 size;

    inline i32 begin() const { return offset; }
    inline i32 end() const { return offset + size; }
};

static i32 Compare(const HeapItem& lhs, const HeapItem& rhs)
{
    return lhs.offset - rhs.offset;
}

static bool Fits(const HeapItem& key, const HeapItem& item)
{
    return item.size >= key.size;
}

static HeapItem Combine(HeapItem lhs, HeapItem rhs)
{
    ASSERT(Adjacent(lhs, rhs));
    return { Min(lhs.offset, rhs.offset), lhs.size + rhs.size };
}

static HeapItem Shrink(HeapItem& item, i32 desiredSize)
{
    ASSERT(desiredSize > 0);
    const i32 remSize = item.size - desiredSize;
    ASSERT(remSize > 0);
    const i32 remOffset = item.offset + desiredSize;
    item.size = desiredSize;
    return { remOffset, remSize };
}

struct Heap
{
    Array<HeapItem> m_items;
    i32 m_size;
    i32 m_allocated;

    void Init(AllocType allocator, i32 size)
    {
        m_items.Init(allocator);
        m_items.Grow() = { 0, size };
        m_size = size;
    }
    void Reset()
    {
        m_items.Reset();
    }
    void Clear()
    {
        m_items.Clear();
        m_items.Grow() = { 0, m_size };
    }

    i32 size() const { return m_size; }
    i32 SizeAllocated() const { return m_allocated; }
    // Usable amount is less due to fragmentation
    i32 SizeFree() const { return m_size - m_allocated; }

    HeapItem Alloc(i32 desiredSize)
    {
        ASSERT(desiredSize > 0);
        Array<HeapItem> items = m_items;
        const i32 i = Find(items, { -1, desiredSize }, { Fits });
        if (i == -1)
        {
            return { -1, -1 };
        }
        HeapItem item = items[i];
        if (item.size > desiredSize)
        {
            items[i] = Shrink(item, desiredSize);
        }
        else
        {
            items.ShiftRemove(i);
        }
        ASSERT(item.size == desiredSize);
        ASSERT(item.offset >= 0);
        ASSERT(item.offset + item.size <= m_size);
        m_items = items;
        return item;
    }

    void Free(HeapItem item)
    {
        ASSERT(item.offset >= 0);
        ASSERT(item.size > 0);
        ASSERT((u32)(item.end()) <= (u32)m_size);

        Array<HeapItem> items = m_items;
        items.PushBack(item);
        const i32 i = PushSort(
            items.begin(),
            items.size(),
            item,
            { Compare });
        const i32 lhs = i - 1;
        const i32 rhs = i + 1;

        if (rhs < items.size())
        {
            if (Adjacent(items[i], items[rhs]))
            {
                items[i] = Combine(items[i], items[rhs]);
                items.ShiftRemove(rhs);
            }
        }

        if (lhs >= 0)
        {
            if (Adjacent(items[lhs], items[i]))
            {
                items[lhs] = Combine(items[lhs], items[i]);
                items.ShiftRemove(i);
            }
        }

        m_items = items;
    }
};
