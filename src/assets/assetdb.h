#pragma once

#include "common/text.h"
#include "common/heap_item.h"
#include "containers/obj_table.h"
#include "containers/array.h"

using AssetText = Text<64>;

struct AssetArgs final
{
    cstr name;
    cstr pack;
    i32 offset;
    i32 size;
};

struct Asset final
{
    AssetText name;
    AssetText pack;
    i32 offset;
    i32 size;
    void* pData;
    i32 refCount;

    Asset(AssetArgs args) :
        name(args.name),
        pack(args.pack),
        offset(args.offset),
        size(args.size),
        pData(0),
        refCount(0)
    {}
    ~Asset()
    {
        Allocator::Free(pData);
        pData = 0;
    }

    Asset(const Asset& other) = delete;
    Asset& operator=(const Asset& other) = delete;
};

using AssetTable = ObjTable<AssetText, Asset, AssetArgs>;
