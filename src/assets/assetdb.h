#pragma once

#include "common/guid.h"
#include "common/guid_util.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/hash_dict.h"
#include "containers/hash_set.h"

struct AssetDb
{
    GuidTable table;                // guid of the asset name
    Array<Guid> packIds;            // guid of the pack path of the asset
    Array<GuidSet> referencedIds;   // guids of referenced asset names
    Array<Slice<u8>> memories;      // deserialized memory of the asset
    Array<i32> refCounts;           // number of active references to the asset

    void Init(AllocType allocator);
    void Reset();

    i32 Add(Guid name, Guid pack, Slice<const Guid> refersTo);
    bool Remove(Guid name);
    void RemoveAt(i32 i);

    i32 size() const { return table.size(); }
    i32 Find(Guid name) const { return table.Find(name); }
    bool Contains(Guid name) const { return table.Contains(name); }
    Guid GetName(i32 i) const { return table[i]; }
    Guid& GetPack(i32 i) { return packIds[i]; }
    GuidSet& GetRefersTo(i32 i) { return referencedIds[i]; }
    Slice<u8>& GetMemory(i32 i) { return memories[i]; }
    i32& GetRefs(i32 i) { return refCounts[i]; }
};
