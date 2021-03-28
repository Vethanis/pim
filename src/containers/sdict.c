#include "containers/sdict.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include <string.h>

static u32 HashKey(const char* key)
{
    ASSERT(key);
    return HashUtil_CreateHash(HashStr(key));
}

void StrDict_New(StrDict* dict, u32 valueSize, EAlloc allocator)
{
    ASSERT(dict);
    ASSERT(valueSize);
    memset(dict, 0, sizeof(*dict));
    dict->valueSize = valueSize;
    dict->allocator = allocator;
}

void StrDict_Del(StrDict* dict)
{
    ASSERT(dict);
    Mem_Free(dict->hashes);
    char** pim_noalias keys = dict->keys;
    const u32 width = dict->width;
    for (u32 i = 0; i < width; ++i)
    {
        Mem_Free(keys[i]);
        keys[i] = NULL;
    }
    Mem_Free(keys);
    Mem_Free(dict->values);
    memset(dict, 0, sizeof(*dict));
}

void StrDict_Clear(StrDict* dict)
{
    ASSERT(dict);
    dict->count = 0;
    const u32 width = dict->width;
    char** pim_noalias keys = dict->keys;
    for (u32 i = 0; i < width; ++i)
    {
        Mem_Free(keys[i]);
        keys[i] = NULL;
    }
    memset(dict->hashes, 0, sizeof(dict->hashes[0]) * width);
    memset(dict->values, 0, dict->valueSize * width);
}

void StrDict_Reserve(StrDict* dict, i32 count)
{
    ASSERT(dict);

    count = count > 16 ? count : 16;
    const u32 newWidth = NextPow2((u32)count) * 2;
    const u32 oldWidth = dict->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    const u32 valueSize = dict->valueSize;
    const EAlloc allocator = dict->allocator;
    ASSERT(valueSize);

    u32* pim_noalias oldHashes = dict->hashes;
    char** pim_noalias oldKeys = dict->keys;
    u8* pim_noalias oldValues = dict->values;

    u32* pim_noalias newHashes = Mem_Calloc(allocator, sizeof(newHashes[0]) * newWidth);
    char** pim_noalias newKeys = Mem_Calloc(allocator, sizeof(newKeys[0]) * newWidth);
    u8* pim_noalias newValues = Mem_Calloc(allocator, valueSize * newWidth);

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
                    newKeys[j] = oldKeys[i];
                    oldKeys[i] = NULL;
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

i32 StrDict_Find(const StrDict* dict, const char* key)
{
    ASSERT(dict);
    if (!key || !key[0])
    {
        return -1;
    }

    const u32 keyHash = HashKey(key);

    u32 width = dict->width;
    const u32 mask = width - 1u;
    const u32* pim_noalias hashes = dict->hashes;
    char const* const* pim_noalias keys = dict->keys;

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
            if (StrCmp(key, PIM_PATH, keys[j]) == 0)
            {
                return (i32)j;
            }
        }
        ++j;
    }

    return -1;
}

bool StrDict_Get(const StrDict* dict, const char* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(valueOut);
    const u32 valueSize = dict->valueSize;
    ASSERT(valueSize > 0);
    memset(valueOut, 0, valueSize);

    const i32 i = StrDict_Find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u8* pim_noalias values = dict->values;
    memcpy(valueOut, values + i * valueSize, valueSize);

    return true;
}

bool StrDict_Set(StrDict* dict, const char* key, const void* value)
{
    ASSERT(dict);
    ASSERT(value);

    const i32 i = StrDict_Find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    u8* pim_noalias values = dict->values;
    ASSERT(valueSize);
    memcpy(values + i * valueSize, value, valueSize);

    return true;
}

bool StrDict_Add(StrDict* dict, const char* key, const void* value)
{
    ASSERT(dict);
    ASSERT(value);

    if (!key || !key[0])
    {
        return false;
    }
    if (StrDict_Find(dict, key) != -1)
    {
        return false;
    }

    StrDict_Reserve(dict, dict->count + 1);
    dict->count++;

    const u32 mask = dict->width - 1u;
    const u32 valueSize = dict->valueSize;
    const u32 keyHash = HashKey(key);

    u32* pim_noalias hashes = dict->hashes;
    char** pim_noalias  keys = dict->keys;
    u8* pim_noalias values = dict->values;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (HashUtil_EmptyOrTomb(hashes[j]))
        {
            hashes[j] = keyHash;
            keys[j] = StrDup(key, dict->allocator);
            memcpy(values + j * valueSize, value, valueSize);
            return true;
        }
        ++j;
    }
}

bool StrDict_Rm(StrDict* dict, const char* key, void* valueOut)
{
    ASSERT(dict);

    const i32 i = StrDict_Find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    ASSERT(valueSize);

    u32* pim_noalias hashes = dict->hashes;
    char** pim_noalias keys = dict->keys;
    u8* pim_noalias values = dict->values;

    if (valueOut)
    {
        memcpy(valueOut, values + i * valueSize, valueSize);
    }

    hashes[i] |= hashutil_tomb_mask;
    Mem_Free(keys[i]);
    keys[i] = NULL;
    memset(values + i * valueSize, 0, valueSize);

    dict->count -= 1;

    return true;
}

typedef struct cmpctx_s
{
    const char** pim_noalias keys;
    const u8* pim_noalias values;
    SDictCmpFn cmp;
    void* pim_noalias usr;
    u32 valueSize;
} cmpctx_t;

static i32 SDictCmp(const void* lhs, const void* rhs, void* usr)
{
    const u32 a = *(u32*)lhs;
    const u32 b = *(u32*)rhs;
    const cmpctx_t* pim_noalias ctx = usr;
    const u8* pim_noalias values = ctx->values;
    const u32 stride = ctx->valueSize;
    return ctx->cmp(
        ctx->keys[a], ctx->keys[b],
        values + a * stride, values + b * stride,
        ctx->usr);
}

u32* StrDict_Sort(const StrDict* dict, SDictCmpFn cmp, void* usr)
{
    ASSERT(dict);
    ASSERT(cmp);
    ASSERT(dict->valueSize);

    const u32 length = dict->count;
    const u32 width = dict->width;
    const u32* hashes = dict->hashes;
    u32* pim_noalias indices = Temp_Calloc(length * sizeof(indices[0]));
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
        .valueSize = dict->valueSize,
        .cmp = cmp,
        .usr = usr,
    };
    QuickSort(indices, length, sizeof(indices[0]), SDictCmp, &ctx);
    return indices;
}

i32 SDictStrCmp(
    const char* lKey, const char* rKey,
    const void* lVal, const void* rVal,
    void* usr)
{
    return StrCmp(lKey, PIM_PATH, rKey);
}
