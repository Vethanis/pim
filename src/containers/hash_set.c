#include "containers/hash_set.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"

#include <string.h>

void HashSet_New(HashSet* set, u32 keySize, EAlloc allocator)
{
    memset(set, 0, sizeof(*set));
    set->stride = keySize;
    set->allocator = allocator;
}

void HashSet_Del(HashSet* set)
{
    Mem_Free(set->hashes);
    set->hashes = NULL;
    Mem_Free(set->keys);
    set->keys = NULL;
    set->count = 0;
    set->width = 0;
}

void HashSet_Clear(HashSet* set)
{
    set->count = 0;
    if (set->hashes)
    {
        memset(set->hashes, 0, sizeof(u32) * set->width);
        memset(set->keys, 0, set->stride * set->width);
    }
}

void HashSet_Reserve(HashSet* set, u32 minCount)
{
    const u32 newWidth = NextPow2(minCount) * 2;
    const u32 oldWidth = set->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    const u32 stride = set->stride;
    u32* newHashes = Mem_Calloc(set->allocator, sizeof(u32) * newWidth);
    u8* newKeys = Mem_Calloc(set->allocator, stride * newWidth);

    u32* oldHashes = set->hashes;
    u8* oldKeys = set->keys;
    const u32 newMask = newWidth - 1u;

    for (u32 i = 0u; i < oldWidth; ++i)
    {
        const u32 hash = oldHashes[i];
        if (HashUtil_Valid(hash))
        {
            u32 j = hash;
            while (true)
            {
                j &= newMask;
                if (!newHashes[j])
                {
                    newHashes[j] = hash;
                    memcpy(newKeys + stride * j, oldKeys + stride * i, stride);
                    break;
                }
                ++j;
            }
        }
    }

    set->hashes = newHashes;
    set->keys = newKeys;
    set->width = newWidth;

    Mem_Free(oldHashes);
    Mem_Free(oldKeys);
}

static i32 hashset_find2(const HashSet* set, u32 keyHash, const void* key, u32 keySize)
{
    const u32* hashes = set->hashes;
    const u8* keys = set->keys;
    ASSERT(set->stride == keySize);

    u32 width = set->width;
    const u32 mask = width - 1u;

    for (u32 j = keyHash; width--; ++j)
    {
        j &= mask;
        const u32 jHash = hashes[j];
        if (HashUtil_Empty(jHash))
        {
            break;
        }
        if (jHash == keyHash)
        {
            const void* heldKey = keys + j * keySize;
            if (!memcmp(key, heldKey, keySize))
            {
                return (i32)j;
            }
        }
    }

    return -1;
}

static i32 hashset_find(const HashSet* set, const void* key, u32 keySize)
{
    const u32 hash = HashUtil_HashBytes(key, keySize);
    return hashset_find2(set, hash, key, keySize);
}

bool HashSet_Contains(const HashSet* set, const void* key, u32 keySize)
{
    return hashset_find(set, key, keySize) != -1;
}

bool HashSet_Add(HashSet* set, const void* key, u32 keySize)
{
    const u32 keyHash = HashUtil_HashBytes(key, keySize);
    if (hashset_find2(set, keyHash, key, keySize) != -1)
    {
        return false;
    }

    HashSet_Reserve(set, set->count + 3u);

    u32* hashes = set->hashes;
    u8* keys = set->keys;
    const u32 mask = set->width - 1u;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (HashUtil_EmptyOrTomb(hashes[j]))
        {
            hashes[j] = keyHash;
            memcpy(keys + j * keySize, key, keySize);
            ++(set->count);
            return true;
        }
        ++j;
    }

    return false;
}

bool HashSet_Rm(HashSet* set, const void* key, u32 keySize)
{
    const i32 i = hashset_find(set, key, keySize);
    if (i != -1)
    {
        set->hashes[i] |= hashutil_tomb_mask;
        u8* keys = set->keys;
        memset(keys + keySize * i, 0, keySize);
        --(set->count);
        return true;
    }
    return false;
}
