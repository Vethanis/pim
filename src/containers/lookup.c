#include "containers/lookup.h"

#include "allocator/allocator.h"
#include "common/nextpow2.h"
#include "containers/hash_util.h"
#include <string.h>

#define kEmpty  -1
#define kTomb   -2

void Lookup_New(Lookup *const loo)
{
    memset(loo, 0, sizeof(*loo));
}

void Lookup_Del(Lookup *const loo)
{
    Mem_Free(loo->indices);
    memset(loo, 0, sizeof(*loo));
}

void Lookup_Reserve(
    Lookup *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    i32 capacity)
{
    const u32 newWidth = NextPow2(capacity) * 4;
    const u32 oldWidth = loo->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    i32 *const pim_noalias oldIndices = loo->indices;
    i32 *const pim_noalias newIndices = Perm_Alloc(sizeof(newIndices[0]) * newWidth);
    for (u32 i = 0; i < newWidth; ++i)
    {
        newIndices[i] = kEmpty;
    }

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth; ++i)
    {
        i32 nameIndex = oldIndices[i];
        if (nameIndex >= 0)
        {
            ASSERT(nameIndex < nameCount);
            u32 j = Guid_HashOf(names[nameIndex]);
            while (true)
            {
                j &= newMask;
                if (newIndices[j] < 0)
                {
                    newIndices[j] = nameIndex;
                    break;
                }
                ++j;
            }
        }
    }

    loo->indices = newIndices;
    loo->width = newWidth;

    Mem_Free(oldIndices);
}

i32 Lookup_Insert(
    Lookup *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    Guid name,
    i32 nameIndex)
{
    ASSERT(nameIndex >= 0);
    ASSERT(nameIndex <= nameCount);
    ASSERT(!Guid_IsNull(name));

    Lookup_Reserve(loo, names, nameCount, nameCount + 1);

    const u32 mask = loo->width - 1u;
    i32 *const pim_noalias indices = loo->indices;
    u32 iLookup = Guid_HashOf(name);
    while (true)
    {
        iLookup &= mask;
        // if empty or tombstone
        if (indices[iLookup] < 0)
        {
            indices[iLookup] = nameIndex;
            return iLookup;
        }
        ++iLookup;
    }
    return -1;
}

bool Lookup_Find(
    Lookup const *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    Guid name,
    i32 *const pim_noalias nameIndexOut,
    i32 *const pim_noalias iLookupOut)
{
    *nameIndexOut = -1;
    *iLookupOut = -1;
    if (Guid_IsNull(name))
    {
        return false;
    }

    const u32 width = loo->width;
    const u32 mask = width - 1u;
    const i32 *const pim_noalias indices = loo->indices;
    const u32 hash = Guid_HashOf(name);

    for (u32 i = 0; i < width; ++i)
    {
        const u32 iLookup = (hash + i) & mask;
        const i32 nameIndex = indices[iLookup];
        // if empty
        if (nameIndex == kEmpty)
        {
            break;
        }
        // if not a tombstone
        if (nameIndex >= 0)
        {
            ASSERT(nameIndex < nameCount);
            if (Guid_Equal(name, names[nameIndex]))
            {
                *nameIndexOut = nameIndex;
                *iLookupOut = (i32)iLookup;
                return true;
            }
        }
    }

    return false;
}

void Lookup_Remove(
    Lookup *const loo,
    i32 keyIndex,
    i32 iLookup)
{
    if (iLookup >= 0)
    {
        ASSERT(keyIndex >= 0);
        ASSERT((u32)iLookup < loo->width);
        loo->indices[iLookup] = kTomb;
    }
}

void Lookup_Clear(Lookup *const loo)
{
    const u32 width = loo->width;
    i32 *const pim_noalias indices = loo->indices;
    for (u32 i = 0; i < width; ++i)
    {
        indices[i] = kEmpty;
    }
}
