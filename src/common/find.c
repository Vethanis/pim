#include "common/find.h"
#include "common/stringutil.h"
#include "containers/hash_util.h"
#include <string.h>

i32 Find(const void* arr, i32 stride, i32 length, const void* key)
{
    ASSERT(arr);
    ASSERT(stride > 0);
    ASSERT(length >= 0);
    ASSERT(key);
    for (i32 i = 0; i < length; ++i)
    {
        if (memcmp((const u8*)arr + i * stride, key, stride) == 0)
        {
            return i;
        }
    }
    return -1;
}

i32 RFind(const void* arr, i32 stride, i32 length, const void* key)
{
    ASSERT(arr);
    ASSERT(stride > 0);
    ASSERT(length >= 0);
    ASSERT(key);
    for (i32 i = length - 1; i >= 0; --i)
    {
        if (memcmp((const u8*)arr + i * stride, key, stride) == 0)
        {
            return i;
        }
    }
    return -1;
}

i32 StrFind(const char** arr, i32 length, const char* key)
{
    ASSERT(arr);
    ASSERT(length >= 0);
    ASSERT(key);
    for (i32 i = 0; i < length; ++i)
    {
        const char* item = arr[i];
        ASSERT(item);
        if (StrCmp(item, PIM_PATH, key) == 0)
        {
            return i;
        }
    }
    return -1;
}

i32 StrRFind(const char** arr, i32 length, const char* key)
{
    ASSERT(arr);
    ASSERT(length >= 0);
    ASSERT(key);
    for (i32 i = length - 1; i >= 0; --i)
    {
        const char* item = arr[i];
        ASSERT(item);
        if (StrCmp(item, PIM_PATH, key) == 0)
        {
            return i;
        }
    }
    return -1;
}

i32 HashFind(const void* arr, i32 stride, const u32* hashes, i32 length, const void* key)
{
    ASSERT(arr);
    ASSERT(stride > 0);
    ASSERT(hashes);
    ASSERT(length >= 0);
    ASSERT(key);
    const u32 hash = HashUtil_HashBytes(key, stride);
    for (i32 i = 0; i < length; ++i)
    {
        if (hashes[i] == hash)
        {
            if (memcmp((const u8*)arr + i * stride, key, stride) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

i32 HashRFind(const void* arr, i32 stride, const u32* hashes, i32 length, const void* key)
{
    ASSERT(arr);
    ASSERT(stride > 0);
    ASSERT(hashes);
    ASSERT(length >= 0);
    ASSERT(key);
    const u32 hash = HashUtil_HashBytes(key, stride);
    for (i32 i = length - 1; i >= 0; --i)
    {
        if (hashes[i] == hash)
        {
            if (memcmp((const u8*)arr + i * stride, key, stride) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

i32 Find_i32(const i32* values, i32 length, i32 key)
{
    ASSERT(values || !length);
    for (i32 i = 0; i < length; ++i)
    {
        if (values[i] == key)
        {
            return i;
        }
    }
    return -1;
}

i32 Find_u32(const u32* values, i32 length, u32 key)
{
    ASSERT(values || !length);
    for (i32 i = 0; i < length; ++i)
    {
        if (values[i] == key)
        {
            return i;
        }
    }
    return -1;
}

i32 Find_u64(const u64* values, i32 length, u64 key)
{
    ASSERT(values || !length);
    for (i32 i = 0; i < length; ++i)
    {
        if (values[i] == key)
        {
            return i;
        }
    }
    return -1;
}

i32 Find_ptr(const void** values, i32 length, const void* key)
{
    ASSERT(values || !length);
    for (i32 i = 0; i < length; ++i)
    {
        if (values[i] == key)
        {
            return i;
        }
    }
    return -1;
}
