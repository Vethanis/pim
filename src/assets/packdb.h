#pragma once

#include "common/guid.h"
#include "common/guid_util.h"
#include "common/text.h"
#include "containers/array.h"
#include "containers/heap.h"

struct PackFile
{
    PathText path;              // path to the file
    Heap heap;                  // free positions in file
    GuidTable table;            // guid table of asset name
    Array<HeapItem> positions;  // positions of asset in file

    // ------------------------------------------------------------------------

    void Init(AllocType allocator);
    void Reset();

    i32 Find(Guid name) const { return table.Find(name); }
    bool Contains(Guid name) const { return table.Contains(name); }

    i32 Add(Guid name, i32 size);
    bool Remove(Guid name);

    i32 Size() const { return table.size(); }
    Guid GetName(i32 i) const { return table[i]; }
    HeapItem& GetPosition(i32 i) { return positions[i]; }
};

struct PackDb
{
    GuidTable table;            // guid table of the pack path
    Array<PackFile> packs;      // assets within the pack

    // ------------------------------------------------------------------------

    void Init(AllocType allocator);
    void Reset();

    i32 Find(Guid name) const { return table.Find(name); }
    bool Contains(Guid name) const { return table.Contains(name); }

    i32 Add(Guid name, cstr path);
    bool Remove(Guid name);

    i32 Size() const { return table.size(); }
    Guid GetName(i32 i) { return table[i]; }
    PackFile& GetPack(i32 i) { return packs[i]; }
};
