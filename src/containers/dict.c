#include "containers/dict.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "common/sort.h"
#include <string.h>

static void dict_setkey(Dict* dict, u32 index, const void* keyIn)
{
    u8 *const pim_noalias items = dict->keys;
    const u32 sz = dict->keySize;
    memcpy(items + index * sz, keyIn, sz);
}
static void dict_getkey(const Dict* dict, u32 index, void* keyOut)
{
    u8 const *const pim_noalias items = dict->keys;
    const u32 sz = dict->keySize;
    memcpy(keyOut, items + index * sz, sz);
}

static void dict_setvalue(Dict* dict, u32 index, const void* valueIn)
{
    u8 *const pim_noalias items = dict->values;
    const u32 sz = dict->valueSize;
    memcpy(items + index * sz, valueIn, sz);
}
static void dict_getvalue(const Dict* dict, u32 index, void* valueOut)
{
    u8 const *const pim_noalias items = dict->values;
    const u32 sz = dict->valueSize;
    memcpy(valueOut, items + index * sz, sz);
}

static u32 dict_insert(Dict* dict, const void* key)
{
    Dict_Reserve(dict, dict->count + 1);
    dict->count++;

    const u32 mask = dict->width - 1u;
    const u32 keySize = dict->keySize;
    const u32 keyHash = HashUtil_HashBytes(key, keySize);

    u32 *const pim_noalias hashes = dict->hashes;
    u8 *const pim_noalias keys = dict->keys;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (HashUtil_EmptyOrTomb(hashes[j]))
        {
            hashes[j] = keyHash;
            memcpy(keys + j * keySize, key, keySize);
            break;
        }
        ++j;
    }

    return j;
}

// ----------------------------------------------------------------------------

void Dict_New(Dict* dict, u32 keySize, u32 valueSize, EAlloc allocator)
{
    ASSERT(dict);
    ASSERT(keySize);
    ASSERT(valueSize);
    memset(dict, 0, sizeof(*dict));
    dict->keySize = keySize;
    dict->valueSize = valueSize;
    dict->allocator = allocator;
}

void Dict_Del(Dict* dict)
{
    ASSERT(dict);

    dict->width = 0;
    dict->count = 0;
    Mem_Free(dict->hashes); dict->hashes = NULL;
    Mem_Free(dict->keys); dict->keys = NULL;
    Mem_Free(dict->values); dict->values = NULL;
}

void Dict_Clear(Dict* dict)
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

void Dict_Reserve(Dict* dict, i32 count)
{
    ASSERT(dict);

    count = count > 16 ? count : 16;
    const u32 newWidth = NextPow2((u32)count) * 2;
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

    u32 *const pim_noalias oldHashes = dict->hashes;
    u8 *const pim_noalias oldKeys = dict->keys;
    u8 *const pim_noalias oldValues = dict->values;

    u32 *const pim_noalias newHashes = Mem_Calloc(allocator, sizeof(u32) * newWidth);
    u8 *const pim_noalias newKeys = Mem_Calloc(allocator, keySize * newWidth);
    u8 *const pim_noalias newValues = Mem_Calloc(allocator, valueSize * newWidth);

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth;)
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

    Mem_Free(oldHashes);
    Mem_Free(oldKeys);
    Mem_Free(oldValues);

    dict->width = newWidth;
    dict->hashes = newHashes;
    dict->keys = newKeys;
    dict->values = newValues;
}

i32 Dict_Find(const Dict* dict, const void* key)
{
    ASSERT(dict);
    ASSERT(key);

    const u32 keySize = dict->keySize;
    ASSERT(keySize);
    const u32 keyHash = HashUtil_HashBytes(key, keySize);

    u32 width = dict->width;
    const u32 mask = width - 1u;
    u32 const *const pim_noalias hashes = dict->hashes;
    u8 const *const pim_noalias keys = dict->keys;

    u32 j = keyHash;
    while (width--)
    {
        j &= mask;
        const u32 jHash = hashes[j];
        if (HashUtil_Empty(jHash))
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

bool Dict_Get(const Dict* dict, const void* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueOut);
    const i32 i = Dict_Find(dict, key);
    if (i >= 0)
    {
        dict_getvalue(dict, i, valueOut);
        return true;
    }
    return false;
}

bool Dict_Set(Dict* dict, const void* key, const void* valueIn)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueIn);
    const i32 i = Dict_Find(dict, key);
    if (i >= 0)
    {
        dict_setvalue(dict, i, valueIn);
        return true;
    }
    return false;
}

bool Dict_Add(Dict* dict, const void* key, const void* valueIn)
{
    ASSERT(dict);
    ASSERT(key);
    ASSERT(valueIn);
    if (Dict_Find(dict, key) >= 0)
    {
        return false;
    }
    u32 j = dict_insert(dict, key);
    dict_setvalue(dict, j, valueIn);
    return true;
}

bool Dict_Rm(Dict* dict, const void* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(key);

    const i32 i = Dict_Find(dict, key);
    if (i >= 0)
    {
        const u32 keySize = dict->keySize;
        const u32 valueSize = dict->valueSize;
        ASSERT(keySize);
        ASSERT(valueSize);

        u32 *const pim_noalias hashes = dict->hashes;
        u8 *const pim_noalias keys = dict->keys;
        u8 *const pim_noalias values = dict->values;

        if (valueOut)
        {
            memcpy(valueOut, values + i * valueSize, valueSize);
        }

        hashes[i] |= hashutil_tomb_mask;
        memset(keys + i * keySize, 0, keySize);
        memset(values + i * valueSize, 0, valueSize);

        dict->count--;

        return true;
    }

    return false;
}

bool Dict_SetAdd(Dict* dict, const void* key, const void* valueIn)
{
    if (Dict_Set(dict, key, valueIn))
    {
        return true;
    }
    u32 j = dict_insert(dict, key);
    dict_setvalue(dict, j, valueIn);
    return false;
}

bool Dict_GetAdd(Dict* dict, const void* key, void* valueOut)
{
    i32 i = Dict_Find(dict, key);
    if (i >= 0)
    {
        dict_getvalue(dict, i, valueOut);
        return true;
    }
    u32 j = dict_insert(dict, key);
    memset(valueOut, 0, dict->valueSize);
    dict_setvalue(dict, j, valueOut);
    return false;
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

u32* Dict_Sort(const Dict* dict, DictCmpFn cmp, void* usr)
{
    ASSERT(dict);
    ASSERT(cmp);
    ASSERT(dict->keySize);
    ASSERT(dict->valueSize);

    const u32 length = dict->count;
    const u32 width = dict->width;
    const u32* hashes = dict->hashes;
    u32* indices = Temp_Calloc(length * sizeof(indices[0]));
    u32 j = 0;
    for (u32 i = 0; i < width; ++i)
    {
        if (HashUtil_Valid(hashes[i]))
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
    QuickSort(indices, length, sizeof(indices[0]), DictCmp, &ctx);
    return indices;
}
