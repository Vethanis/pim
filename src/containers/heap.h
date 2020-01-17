#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "common/heap_item.h"
#include "common/sort.h"
#include "containers/array.h"

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

    i32 Find(i32 desiredSize) const
    {
        const HeapItem* const items = m_items.begin();
        const i32 count = m_items.size();
        for (i32 i = 0; i < count; ++i)
        {
            if (items[i].size >= desiredSize)
            {
                return i;
            }
        }
        return -1;
    }

    HeapItem Alloc(i32 desiredSize)
    {
        ASSERT(desiredSize > 0);
        const i32 i = Find(desiredSize);
        if (i == -1)
        {
            return { -1, -1 };
        }
        Array<HeapItem> items = m_items;
        HeapItem item = items[i];
        if (item.size > desiredSize)
        {
            items[i] = Split(item, desiredSize);
        }
        else
        {
            items.ShiftRemove(i);
        }
        ASSERT(item.size == desiredSize);
        ASSERT(item.offset >= 0);
        ASSERT(item.offset + item.size <= m_size);
        m_items = items;
        m_allocated += item.size;
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
        m_allocated -= item.size;
    }
};
