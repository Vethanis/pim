#pragma once

#include "common/guid.h"
#include "common/guid_util.h"
#include "common/text.h"
#include "containers/array.h"
#include "containers/heap.h"

struct PackAssets
{
    GuidTable table;            // guid table of asset name
    Array<HeapItem> positions;  // positions of asset in file

    void Init(AllocType allocator);
    void Reset();

    i32 Add(Guid name, HeapItem position);
    bool Remove(Guid name);
    void RemoveAt(i32 i);

    i32 size() const { return table.size(); }
    i32 Find(Guid name) const { return table.Find(name); }
    bool Contains(Guid name) const { return table.Contains(name); }
    Guid GetName(i32 i) const { return table[i]; }
    HeapItem& GetPosition(i32 i) { return positions[i]; }
};

struct PackDb
{
    GuidTable table;            // guid table of the pack path
    Array<PathText> paths;      // path to pack
    Array<Heap> heaps;          // free positions in pack
    Array<PackAssets> packs;    // the locations of the assets in the pack

    void Init(AllocType allocator);
    void Reset();

    i32 Add(Guid name, cstr path);
    bool Remove(Guid name);
    void RemoveAt(i32 i);

    i32 size() const { return table.size(); }
    i32 Find(Guid name) const { return table.Find(name); }
    bool Contains(Guid name) const { return table.Contains(name); }
    Guid GetName(i32 i) const { return table[i]; }
    PathText& GetPath(i32 i) { return paths[i]; }
    Heap& GetHeap(i32 i) { return heaps[i]; }
    PackAssets& GetPack(i32 i) { return packs[i]; }
};
