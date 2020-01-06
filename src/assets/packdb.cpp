#include "assets/packdb.h"

// ----------------------------------------------------------------------------

void PackAssets::Init(AllocType allocator)
{
    table.Init(allocator);
    positions.Init(allocator);
}

void PackAssets::Reset()
{
    table.Reset();
    positions.Reset();
}

i32 PackAssets::Add(Guid name, HeapItem position)
{
    const i32 i = table.Add(name);
    if (i != -1)
    {
        positions.PushBack(position);
    }
    return i;
}

bool PackAssets::Remove(Guid name)
{
    const i32 i = Find(name);
    if (i != -1)
    {
        RemoveAt(i);
        return true;
    }
    return false;
}

void PackAssets::RemoveAt(i32 i)
{
    table.RemoveAt(i);
    positions.Remove(i);
}

// ----------------------------------------------------------------------------

void PackDb::Init(AllocType allocator)
{
    table.Init(allocator);
    paths.Init(allocator);
    heaps.Init(allocator);
    packs.Init(allocator);
}

void PackDb::Reset()
{
    for (Heap& heap : heaps)
    {
        heap.Reset();
    }
    for (PackAssets& pack : packs)
    {
        pack.Reset();
    }

    table.Reset();
    paths.Reset();
    heaps.Reset();
    packs.Reset();
}

i32 PackDb::Add(Guid name, cstr path)
{
    const i32 i = table.Add(name);
    if (i != -1)
    {
        paths.PushBack(PathText(path));
        heaps.Grow().Init(table.GetAllocator(), 64 << 20);
        packs.Grow().Init(table.GetAllocator());
    }
    return i;
}

bool PackDb::Remove(Guid name)
{
    const i32 i = table.Find(name);
    if (i != -1)
    {
        RemoveAt(i);
        return true;
    }
    return false;
}

void PackDb::RemoveAt(i32 i)
{
    heaps[i].Reset();
    packs[i].Reset();

    table.RemoveAt(i);
    paths.Remove(i);
    heaps.Remove(i);
    packs.Remove(i);
}
