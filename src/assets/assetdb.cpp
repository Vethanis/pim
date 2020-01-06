#include "assets/assetdb.h"

void AssetDb::Init(AllocType allocator)
{
    table.Init(allocator);
    packIds.Init(allocator);
    referencedIds.Init(allocator);
    memories.Init(allocator);
    refCounts.Init(allocator);
}

void AssetDb::Reset()
{
    for (GuidSet& set : referencedIds)
    {
        set.Reset();
    }
    for (Slice<u8> memory : memories)
    {
        Allocator::Free(memory.begin());
    }

    table.Reset();
    packIds.Reset();
    referencedIds.Reset();
    memories.Reset();
    refCounts.Reset();
}

i32 AssetDb::Add(Guid name, Guid pack, Slice<const Guid> refersTo)
{
    const i32 i = table.Add(name);
    if (i != -1)
    {
        packIds.Grow() = pack;
        referencedIds.Grow() = GuidSet::Build(table.GetAllocator(), refersTo);
        memories.Grow() = {};
        refCounts.Grow() = 0;
    }
    return i;
}

bool AssetDb::Remove(Guid name)
{
    const i32 i = Find(name);
    if (i == -1)
    {
        return false;
    }
    RemoveAt(i);
    return true;
}

void AssetDb::RemoveAt(i32 i)
{
    referencedIds[i].Reset();
    Allocator::Free(memories[i].begin());

    table.RemoveAt(i);
    packIds.Remove(i);
    referencedIds.Remove(i);
    memories.Remove(i);
    refCounts.Remove(i);
}
