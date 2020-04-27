#include "containers/dict.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "common/sort.h"

#include <string.h>

static u32 HashKey(const void* key, u32 keySize)
{
    ASSERT(key);
    ASSERT(keySize);
    return hashutil_create_hash(Fnv32Bytes(key, keySize, Fnv32Bias));
}

void dict_new(dict_t* dict, u32 keySize, u32 valueSize, EAlloc allocator)
{
    ASSERT(dict);
    ASSERT(valueSize);
    memset(dict, 0, sizeof(*dict));
    dict->keySize = keySize;
    dict->valueSize = valueSize;
    dict->allocator = allocator;
}

void dict_del(dict_t* dict)
{
    ASSERT(dict);

    dict->width = 0;
    dict->count = 0;
    pim_free(dict->hashes);
    dict->hashes = NULL;
    pim_free(dict->keys);
    dict->keys = NULL;
    pim_free(dict->values);
    dict->values = NULL;
}

void dict_clear(dict_t* dict)
{
    ASSERT(dict);
    dict->count = 0;
    const u32 width = dict->width;
    if (width)
    {
        memset(dict->hashes, 0, sizeof(dict->hashes[0]) * width);
        memset(dict->keys, 0, dict->keySize * width);
        memset(dict->values, 0, dict->valueSize * width);
    }
}

void dict_reserve(dict_t* dict, i32 count)
{
    ASSERT(dict);

    count = count > 16 ? count : 16;
    const u32 newWidth = NextPow2((u32)count);
    const u32 oldWidth = dict->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    const u32 keySize = dict->keySize;
    const u32 valueSize = dict->valueSize;
    const EAlloc allocator = dict->allocator;
    ASSERT(keySize);
    ASSERT(valueSize);

    u32* oldHashes = dict->hashes;
    u8* oldKeys = dict->keys;
    u8* oldValues = dict->values;

    u32* newHashes = pim_calloc(allocator, sizeof(u32) * newWidth);
    u8* newKeys = pim_calloc(allocator, keySize * newWidth);
    u8* newValues = pim_calloc(allocator, valueSize * newWidth);

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth;)
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
                    memcpy(newKeys + j * keySize, oldKeys + i * keySize, keySize);
                    memcpy(newValues + j * valueSize, oldValues + i * valueSize, valueSize);
                    goto next;
                }
                ++j;
            }
        }
    next:
        ++i;
    }

    pim_free(oldHashes);
    pim_free(oldKeys);
    pim_free(oldValues);

    dict->width = newWidth;
    dict->hashes = newHashes;
    dict->keys = newKeys;
    dict->values = newValues;
}

i32 dict_find(const dict_t* dict, const void* key)
{
    ASSERT(dict);
    ASSERT(key);

    const u32 keySize = dict->keySize;
    const u32 keyHash = HashKey(key, keySize);

    u32 width = dict->width;
    const u32 mask = width - 1u;
    const u32* hashes = dict->hashes;
    const u8* keys = dict->keys;

    u32 j = keyHash;
    while (width--)
    {
        j &= mask;
        const u32 jHash = hashes[j];
        if (hashutil_empty(jHash))
        {
            break;
        }
        if (jHash == keyHash)
        {
            if (!memcmp(key, keys + j * keySize, keySize))
            {
                return (i32)j;
            }
        }
        ++j;
    }

    return -1;
}

bool dict_get(dict_t* dict, const void* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueOut);

    const i32 i = dict_find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    const u8* values = dict->values;
    ASSERT(valueSize);
    memcpy(valueOut, values + i * valueSize, valueSize);

    return true;
}

bool dict_set(dict_t* dict, const void* key, const void* valueIn)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueIn);

    const i32 i = dict_find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    u8* values = dict->values;
    ASSERT(valueSize);
    memcpy(values + i * valueSize, valueIn, valueSize);

    return true;
}

bool dict_add(dict_t* dict, const void* key, const void* valueIn)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueIn);

    if (dict_find(dict, key) != -1)
    {
        return false;
    }
    dict_reserve(dict, dict->count + 1);
    dict->count++;

    const u32 mask = dict->width - 1u;
    const u32 valueSize = dict->valueSize;
    const u32 keySize = dict->keySize;
    const u32 keyHash = HashKey(key, keySize);

    u32* hashes = dict->hashes;
    u8* keys = dict->keys;
    u8* values = dict->values;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (hashutil_empty_or_tomb(hashes[j]))
        {
            hashes[j] = keyHash;
            memcpy(keys + j * keySize, key, keySize);
            memcpy(values + j * valueSize, valueIn, valueSize);
            return true;
        }
        ++j;
    }
}

bool dict_rm(dict_t* dict, const void* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueOut);

    const i32 i = dict_find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    const u32 keySize = dict->keySize;
    ASSERT(valueSize);

    u32* hashes = dict->hashes;
    u8* keys = dict->keys;
    u8* values = dict->values;

    memcpy(valueOut, values + i * valueSize, valueSize);

    hashes[i] |= hashutil_tomb_mask;
    memset(keys + i * keySize, 0, keySize);
    memset(values + i * valueSize, 0, valueSize);

    dict->count--;

    return true;
}

typedef struct cmpctx_s
{
    const u8* keys;
    const u8* values;
    u32 keySize;
    u32 valueSize;
    DictCmpFn cmp;
    void* usr;
} cmpctx_t;

static i32 DictCmp(const void* lhs, const void* rhs, void* usr)
{
    const u32 a = *(u32*)lhs;
    const u32 b = *(u32*)rhs;
    const cmpctx_t* ctx = usr;
    const u8* keys = ctx->keys;
    const u8* values = ctx->values;
    const u32 keySize = ctx->keySize;
    const u32 valueSize = ctx->valueSize;
    return ctx->cmp(
        keys + a * keySize, keys + b * keySize,
        values + a * valueSize, values + b * valueSize,
        ctx->usr);
}

u32* dict_sort(const dict_t* dict, DictCmpFn cmp, void* usr)
{
    ASSERT(dict);
    ASSERT(cmp);
    ASSERT(dict->keySize);
    ASSERT(dict->valueSize);

    const u32 length = dict->count;
    const u32 width = dict->width;
    const u32* hashes = dict->hashes;
    u32* indices = pim_malloc(EAlloc_Temp, length * sizeof(indices[0]));
    u32 j = 0;
    for (u32 i = 0; i < width; ++i)
    {
        if (hashutil_valid(hashes[i]))
        {
            indices[j++] = i;
        }
    }
    ASSERT(j == length);
    cmpctx_t ctx =
    {
        .keys = dict->keys,
        .values = dict->values,
        .keySize = dict->keySize,
        .valueSize = dict->valueSize,
        .cmp = cmp,
        .usr = usr,
    };
    pimsort(indices, length, sizeof(indices[0]), DictCmp, &ctx);
    return indices;
}
