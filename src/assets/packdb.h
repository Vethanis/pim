#pragma once

#include "common/guid_util.h"
#include "common/text.h"
#include "containers/heap.h"
#include "containers/obj_table.h"

struct Pack
{
    PathText path;
    Heap heap;

    Pack() {}
    ~Pack()
    {
        heap.Reset();
    }

    void Init(cstr pathStr, i32 fileSize)
    {
        path = pathStr;
        heap.Init(Alloc_Pool, fileSize);
    }

    HeapItem Alloc(i32 size)
    {
        return heap.Alloc(size);
    }
    void Free(HeapItem item)
    {
        heap.Free(item);
    }
    void Remove(HeapItem extant)
    {
        bool removed = heap.Remove(extant);
        ASSERT(removed);
    }
};

using PackTable = ObjTable<Guid, Pack, GuidComparator>;
