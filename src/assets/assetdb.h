#pragma once

#include "common/text.h"
#include "common/heap_item.h"
#include "common/guid_util.h"
#include "containers/obj_table.h"

struct Asset
{
    Text<64> name;
    Guid pack;
    GuidSet children;
    HeapItem position;
    Slice<u8> memory;
    i32 refCount;

    Asset() {}
    ~Asset()
    {
        children.Reset();
        Allocator::Free(memory.begin());
    }
    Asset(const Asset& other) = delete;
    Asset& operator=(const Asset& other) = delete;

    void Init(cstr nameStr, Guid packId, HeapItem pos)
    {
        name = nameStr;
        pack = packId;
        position = pos;
        children.Init(Alloc_Pool);
    }
};

using AssetTable = ObjTable<Guid, Asset, GuidComparator>;
