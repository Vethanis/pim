#include "containers/sdict.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include <string.h>

static u32 HashKey(const char* key)
{
    ASSERT(key);
    return hashutil_create_hash(HashStr(key));
}

void sdict_new(sdict_t* dict, u32 valueSize, EAlloc allocator)
{
    ASSERT(dict);
    ASSERT(valueSize);
    memset(dict, 0, sizeof(*dict));
    dict->valueSize = valueSize;
    dict->allocator = allocator;
}

void sdict_del(sdict_t* dict)
{
    ASSERT(dict);
    pim_free(dict->hashes);
    char** keys = dict->keys;
    u32 width = dict->width;
    for (u32 i = 0; i < width; ++i)
    {
        pim_free(keys[i]);
        keys[i] = NULL;
    }
    pim_free(keys);
    pim_free(dict->values);
    memset(dict, 0, sizeof(*dict));
}

void sdict_clear(sdict_t* dict)
{
    ASSERT(dict);
    dict->count = 0;
    u32 width = dict->width;
    char** keys = dict->keys;
    for (u32 i = 0; i < width; ++i)
    {
        pim_free(keys[i]);
        keys[i] = NULL;
    }
    memset(dict->hashes, 0, sizeof(dict->hashes[0]) * width);
    memset(dict->values, 0, dict->valueSize * width);
}

void sdict_reserve(sdict_t* dict, i32 count)
{
    ASSERT(dict);

    count = count > 16 ? count : 16;
    const u32 newWidth = NextPow2((u32)count);
    const u32 oldWidth = dict->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    const u32 valueSize = dict->valueSize;
    const EAlloc allocator = dict->allocator;
    ASSERT(valueSize);

    u32* oldHashes = dict->hashes;
    char** oldKeys = dict->keys;
    u8* oldValues = dict->values;

    u32* newHashes = pim_calloc(allocator, sizeof(u32) * newWidth);
    char** newKeys = pim_calloc(allocator, sizeof(newKeys[0]) * newWidth);
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

    pim_free(oldHashes);
    pim_free(oldKeys);
    pim_free(oldValues);

    dict->width = newWidth;
    dict->hashes = newHashes;
    dict->keys = newKeys;
    dict->values = newValues;
}

i32 sdict_find(const sdict_t* dict, const char* key)
{
    ASSERT(dict);
    if (!key || !key[0])
    {
        return -1;
    }

    const u32 keyHash = HashKey(key);

    u32 width = dict->width;
    const u32 mask = width - 1u;
    const u32* hashes = dict->hashes;
    const char** keys = dict->keys;

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
            if (StrCmp(key, PIM_PATH, keys[j]) == 0)
            {
                return (i32)j;
            }
        }
        ++j;
    }

    return -1;
}

bool sdict_get(const sdict_t* dict, const char* key, void* valueOut)
{
    ASSERT(dict);
    ASSERT(valueOut);

    const i32 i = sdict_find(dict, key);
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

bool sdict_set(sdict_t* dict, const char* key, const void* value)
{
    ASSERT(dict);
    ASSERT(value);

    const i32 i = sdict_find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    u8* values = dict->values;
    ASSERT(valueSize);
    memcpy(values + i * valueSize, value, valueSize);

    return true;
}

bool sdict_add(sdict_t* dict, const char* key, const void* value)
{
    ASSERT(dict);
    ASSERT(value);

    if (!key || !key[0])
    {
        return false;
    }
    if (sdict_find(dict, key) != -1)
    {
        return false;
    }

    sdict_reserve(dict, dict->count + 1);
    dict->count++;

    const u32 mask = dict->width - 1u;
    const u32 valueSize = dict->valueSize;
    const u32 keyHash = HashKey(key);

    u32* hashes = dict->hashes;
    char** keys = dict->keys;
    u8* values = dict->values;

    u32 j = keyHash;
    while (true)
    {
        j &= mask;
        if (hashutil_empty_or_tomb(hashes[j]))
        {
            hashes[j] = keyHash;
            keys[j] = StrDup(key, dict->allocator);
            memcpy(values + j * valueSize, value, valueSize);
            return true;
        }
        ++j;
    }
}

bool sdict_rm(sdict_t* dict, const char* key, void* valueOut)
{
    ASSERT(dict);

    const i32 i = sdict_find(dict, key);
    if (i == -1)
    {
        return false;
    }

    const u32 valueSize = dict->valueSize;
    ASSERT(valueSize);

    u32* hashes = dict->hashes;
    char** keys = dict->keys;
    u8* values = dict->values;

    if (valueOut)
    {
        memcpy(valueOut, values + i * valueSize, valueSize);
    }

    hashes[i] |= hashutil_tomb_mask;
    pim_free(keys[i]);
    keys[i] = NULL;
    memset(values + i * valueSize, 0, valueSize);

    dict->count -= 1;

    return true;
}

typedef struct cmpctx_s
{
    const char** keys;
    const u8* values;
    SDictCmpFn cmp;
    void* usr;
    u32 valueSize;
} cmpctx_t;

static i32 SDictCmp(const void* lhs, const void* rhs, void* usr)
{
    const u32 a = *(u32*)lhs;
    const u32 b = *(u32*)rhs;
    const cmpctx_t* ctx = usr;
    const u8* values = ctx->values;
    const u32 stride = ctx->valueSize;
    return ctx->cmp(
        ctx->keys[a], ctx->keys[b],
        values + a * stride, values + b * stride,
        ctx->usr);
}

u32* sdict_sort(const sdict_t* dict, SDictCmpFn cmp, void* usr)
{
    ASSERT(dict);
    ASSERT(cmp);
    ASSERT(dict->valueSize);

    const u32 length = dict->count;
    const u32 width = dict->width;
    const u32* hashes = dict->hashes;
    u32* indices = tmp_calloc(length * sizeof(indices[0]));
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
        .valueSize = dict->valueSize,
        .cmp = cmp,
        .usr = usr,
    };
    pimsort(indices, length, sizeof(indices[0]), SDictCmp, &ctx);
    return indices;
}

i32 SDictStrCmp(
    const char* lKey, const char* rKey,
    const void* lVal, const void* rVal,
    void* usr)
{
    return StrCmp(lKey, PIM_PATH, rKey);
}
