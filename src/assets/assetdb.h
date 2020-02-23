#pragma once

#include "common/text.h"
#include "common/guid.h"
#include "common/heap_item.h"
#include "containers/obj_table.h"
#include "containers/hash_set.h"

struct AssetArgs
{
    cstr name;
    Guid pack;
    HeapItem pos;
};

struct Asset
{
    Text<64> name;
    Guid pack;
    HashSet<Guid> children;
    HeapItem position;
    Slice<u8> memory;
    i32 refCount;

    Asset(AssetArgs args) :
        name(args.name),
        pack(args.pack),
        position(args.pos),
        memory({ 0, 0 }),
        refCount(0)
    {
        children.Init(Alloc_Perm);
    }
    ~Asset()
    {
        children.Reset();
        Allocator::Free(memory.begin());
    }
    Asset(const Asset& other) = delete;
    Asset& operator=(const Asset& other) = delete;
};

using AssetTable = ObjTable<Guid, Asset, AssetArgs>;
