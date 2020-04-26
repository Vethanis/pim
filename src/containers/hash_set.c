#include "containers/hash_set.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "common/pimcpy.h"

void hashset_new(hashset_t* set, u32 keySize, EAlloc allocator)
{
    pimset(set, 0, sizeof(*set));
    set->stride = keySize;
    set->allocator = allocator;
}

void hashset_del(hashset_t* set)
{
    pim_free(set->hashes);
    set->hashes = NULL;
    pim_free(set->keys);
    set->keys = NULL;
    set->count = 0;
    set->width = 0;
}

void hashset_clear(hashset_t* set)
{
    set->count = 0;
    if (set->hashes)
    {
        pimset(set->hashes, 0, sizeof(u32) * set->width);
        pimset(set->keys, 0, set->stride * set->width);
    }
}

void hashset_reserve(hashset_t* set, u32 minCount)
{
    const u32 newWidth = NextPow2(minCount);
    const u32 oldWidth = set->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    const u32 stride = set->stride;
    u32* newHashes = pim_calloc(set->allocator, sizeof(u32) * newWidth);
    u8* newKeys = pim_calloc(set->allocator, stride * newWidth);

    u32* oldHashes = set->hashes;
    u8* oldKeys = set->keys;
    const u32 oldMask = oldWidth - 1u;
    const u32 newMask = newWidth - 1u;

    for (u32 i = 0u; i < oldWidth; ++i)
    {
        const u32 hash = oldHashes[i];
        if (hashutil_valid(hash))
        {
            u32 j = hash;
            while (true)
            {
                j &= newMask;
                if (!newHashes[j])
                {
                    newHashes[j] = hash;
                    pimcpy(newKeys + stride * j, oldKeys + stride * i, stride);
                    break;
                }
                ++j;
            }
        }
    }

    set->hashes = newHashes;
    set->keys = newKeys;
    set->width = newWidth;

    pim_free(oldHashes);
    pim_free(oldKeys);
}

static i32 hashset_find2(const hashset_t* set, u32 keyHash, const void* key, u32 keySize)
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
        if (hashutil_empty(jHash))
        {
            break;
        }
        if (jHash == keyHash)
        {
            const void* heldKey = keys + j * keySize;
            if (!pimcmp(key, heldKey, keySize))
            {
                return (i32)j;
            }
        }
    }

    return -1;
}

static i32 hashset_find(const hashset_t* set, const void* key, u32 keySize)
{
    const u32 hash = hashutil_hash(key, keySize);
    return hashset_find2(set, hash, key, keySize);
}

bool hashset_contains(const hashset_t* set, const void* key, u32 keySize)
{
    return hashset_find(set, key, keySize) != -1;
}

bool hashset_add(hashset_t* set, const void* key, u32 keySize)
{
    const u32 keyHash = hashutil_hash(key, keySize);
    if (hashset_find2(set, keyHash, key, keySize) != -1)
    {
        return false;
    }

    hashset_reserve(set, set->count + 3u);

    u32* hashes = set->hashes;
    u8* keys = set->keys;
    const u32 mask = set->width - 1u;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (hashutil_empty_or_tomb(hashes[j]))
        {
            hashes[j] = keyHash;
            pimcpy(keys + j * keySize, key, keySize);
            ++(set->count);
            return true;
        }
        ++j;
    }

    return false;
}

bool hashset_rm(hashset_t* set, const void* key, u32 keySize)
{
    const i32 i = hashset_find(set, key, keySize);
    if (i != -1)
    {
        set->hashes[i] = hashutil_tomb_mask;
        u8* keys = set->keys;
        pimset(keys + keySize * i, 0, keySize);
        --(set->count);
        return true;
    }
    return false;
}
