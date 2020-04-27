#pragma once

#include "containers/carray.h"
#include "allocator/allocator.h"
#include "common/pimcpy.h"

static bool InRange(const arr_t* arr, i32 i)
{
    return (u32)i < (u32)(arr->length);
}

void arr_new(arr_t* arr, i32 sizeOf, EAlloc allocator)
{
    ASSERT(arr);
    ASSERT(sizeOf > 0);
    arr->ptr = NULL;
    arr->length = 0;
    arr->capacity = 0;
    arr->allocator = allocator;
    arr->stride = sizeOf;
}

void arr_del(arr_t* arr)
{
    ASSERT(arr);
    pim_free(arr->ptr);
    arr->ptr = NULL;
    arr->length = 0;
    arr->capacity = 0;
}

void arr_clear(arr_t* arr)
{
    ASSERT(arr);
    arr->length = 0;
}

void arr_reserve(arr_t* arr, i32 minCap)
{
    ASSERT(arr);
    ASSERT(minCap >= 0);
    i32 cur = arr->capacity;
    if (cur < minCap)
    {
        i32 next = cur * 2;
        next = next > 16 ? next : 16;
        next = next > minCap ? next : minCap;
        const i32 stride = arr->stride;
        const i32 length = arr->length;
        void* pNew = pim_malloc(arr->allocator, next * stride);
        void* pOld = arr->ptr;
        pimcpy(pNew, pOld, length * stride);
        pim_free(pOld);
        arr->ptr = pNew;
        arr->capacity = next;
    }
}

void arr_resize(arr_t* arr, i32 length)
{
    ASSERT(arr);
    arr_reserve(arr, length);
    arr->length = length;
}

i32 arr_len(const arr_t* arr)
{
    ASSERT(arr);
    return arr->length;
}

void* arr_at(arr_t* arr, i32 i, i32 sizeOf)
{
    ASSERT(arr);
    ASSERT(arr->stride == sizeOf);
    ASSERT(InRange(arr, i));
    return (u8*)(arr->ptr) + i * sizeOf;
}

void arr_get(const arr_t* arr, i32 i, void* dst)
{
    ASSERT(arr);
    ASSERT(arr->stride);
    ASSERT(InRange(arr, i));
    ASSERT(dst);
    const u8* ptr = (const u8*)(arr->ptr);
    pimcpy(dst, ptr + i * arr->stride, arr->stride);
}

void arr_set(arr_t* arr, i32 i, const void* src)
{
    ASSERT(arr);
    ASSERT(arr->stride);
    ASSERT(InRange(arr, i));
    ASSERT(src);
    u8* ptr = (u8*)(arr->ptr);
    pimcpy(ptr + i * arr->stride, src, arr->stride);
}

void arr_add(arr_t* arr, const void* src, i32 sizeOf)
{
    ASSERT(arr);
    ASSERT(arr->stride == sizeOf);
    const i32 back = arr->length;
    arr_reserve(arr, back + 1);
    arr->length = back + 1;
    u8* dst = (u8*)(arr->ptr) + back * sizeOf;
    pimcpy(dst, src, sizeOf);
}

void arr_dynadd(arr_t* arr, const void* src, i32 bytes)
{
    ASSERT(arr);
    ASSERT(arr->stride == 1);
    const i32 back = arr->length;
    arr_reserve(arr, back + bytes);
    arr->length = back + bytes;
    u8* dst = (u8*)(arr->ptr) + back;
    pimcpy(dst, src, bytes);
}

void arr_pop(arr_t* arr)
{
    ASSERT(arr);
    i32 back = --(arr->length);
    ASSERT(back >= 0);
}

void arr_remove(arr_t* arr, i32 i)
{
    ASSERT(arr);
    ASSERT(InRange(arr, i));
    const i32 back = --(arr->length);
    const i32 stride = arr->stride;
    u8* ptr = (u8*)(arr->ptr);
    pimcpy(ptr + i * stride, ptr + back * stride, stride);
}

i32 arr_find(const arr_t* arr, const void* pKey, i32 sizeOf)
{
    ASSERT(arr);
    ASSERT(pKey);
    ASSERT(arr->stride == sizeOf);
    const u8* ptr = (const u8*)(arr->ptr);
    const i32 len = arr->length;
    for (i32 i = 0; i < len; ++i)
    {
        if (!pimcmp(pKey, ptr + i * sizeOf, sizeOf))
        {
            return i;
        }
    }
    return -1;
}

i32 arr_rfind(const arr_t* arr, const void* pKey, i32 sizeOf)
{
    ASSERT(arr);
    ASSERT(pKey);
    ASSERT(arr->stride == sizeOf);
    const u8* ptr = (const u8*)(arr->ptr);
    const i32 len = arr->length;
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (!pimcmp(pKey, ptr + i * sizeOf, sizeOf))
        {
            return i;
        }
    }
    return -1;
}

bool arr_findadd(arr_t* arr, const void* pKey, i32 sizeOf)
{
    i32 i = arr_rfind(arr, pKey, sizeOf);
    if (i == -1)
    {
        arr_add(arr, pKey, sizeOf);
        return true;
    }
    return false;
}

bool arr_findrm(arr_t* arr, const void* pKey, i32 sizeOf)
{
    i32 i = arr_rfind(arr, pKey, sizeOf);
    if (i != -1)
    {
        arr_remove(arr, i);
        return true;
    }
    return false;
}
