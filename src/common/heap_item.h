#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"

struct HeapItem
{
    i32 offset;
    i32 size;

    inline i32 begin() const { return offset; }
    inline i32 end() const { return offset + size; }

    bool operator<(HeapItem rhs) const
    {
        return offset < rhs.offset;
    }
    bool operator==(HeapItem rhs) const
    {
        return ((offset - rhs.offset) | (size - rhs.size)) == 0;
    }
};

static HeapItem Combine(HeapItem lhs, HeapItem rhs)
{
    ASSERT(Adjacent(lhs, rhs));
    return { Min(lhs.offset, rhs.offset), lhs.size + rhs.size };
}

static HeapItem Split(HeapItem& item, i32 desiredSize)
{
    ASSERT(desiredSize > 0);
    const i32 remSize = item.size - desiredSize;
    ASSERT(remSize > 0);
    const i32 remOffset = item.offset + desiredSize;
    item.size = desiredSize;
    return { remOffset, remSize };
}
