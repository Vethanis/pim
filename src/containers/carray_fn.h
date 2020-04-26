#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "containers/carray.h"
#include "allocator/allocator.h"
#include "common/pimcpy.h"

pim_inline void arr_new(arr_t* arr, i32 sizeOf, EAlloc allocator)
{
    arr->ptr = NULL;
    arr->length = 0;
    arr->capacity = 0;
    arr->allocator = allocator;
    arr->stride = sizeOf;
}

pim_inline void arr_del(arr_t* arr)
{
    pim_free(arr->ptr);
    arr->ptr = NULL;
    arr->length = 0;
    arr->capacity = 0;
}

pim_inline void arr_clear(arr_t* arr)
{
    arr->length = 0;
}

pim_inline void arr_reserve(arr_t* arr, i32 minCap)
{
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

pim_inline void arr_resize(arr_t* arr, i32 length)
{
    arr_reserve(arr, length);
    arr->length = length;
}

pim_inline void arr_add(arr_t* arr, const void* src, i32 sizeOf)
{
    ASSERT(arr->stride == sizeOf);
    const i32 back = arr->length;
    arr_reserve(arr, back + 1);
    arr->length = back + 1;
    u8* dst = (u8*)(arr->ptr) + back * sizeOf;
    pimcpy(dst, src, sizeOf);
}

pim_inline void arr_dynadd(arr_t* arr, const void* src, i32 bytes)
{
    ASSERT(arr->stride == 1);
    const i32 back = arr->length;
    arr_reserve(arr, back + bytes);
    arr->length = back + bytes;
    u8* dst = (u8*)(arr->ptr) + back;
    pimcpy(dst, src, bytes);
}

pim_inline void arr_pop(arr_t* arr)
{
    i32 back = --(arr->length);
    ASSERT(back >= 0);
}

pim_inline void arr_remove(arr_t* arr, i32 i)
{
    const i32 back = --(arr->length);
    const i32 stride = arr->stride;
    u8* ptr = (u8*)(arr->ptr);
    ASSERT((u32)i <= (u32)back);
    pimcpy(ptr + i * stride, ptr + back * stride, stride);
}

pim_inline i32 arr_find(arr_t arr, const void* pKey, i32 sizeOf)
{
    ASSERT(arr.stride == sizeOf);
    const u8* ptr = (const u8*)(arr.ptr);
    const i32 len = arr.length;
    for (i32 i = 0; i < len; ++i)
    {
        if (!pimcmp(pKey, ptr + i * sizeOf, sizeOf))
        {
            return i;
        }
    }
    return -1;
}

pim_inline i32 arr_rfind(arr_t arr, const void* pKey, i32 sizeOf)
{
    ASSERT(arr.stride == sizeOf);
    const u8* ptr = (const u8*)(arr.ptr);
    const i32 len = arr.length;
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (!pimcmp(pKey, ptr + i * sizeOf, sizeOf))
        {
            return i;
        }
    }
    return -1;
}

pim_inline bool arr_findadd(arr_t* arr, const void* pKey, i32 sizeOf)
{
    i32 i = arr_rfind(*arr, pKey, sizeOf);
    if (i == -1)
    {
        arr_add(arr, pKey, sizeOf);
        return true;
    }
    return false;
}

pim_inline bool arr_findrm(arr_t* arr, const void* pKey, i32 sizeOf)
{
    i32 i = arr_rfind(*arr, pKey, sizeOf);
    if (i != -1)
    {
        arr_remove(arr, i);
        return true;
    }
    return false;
}

PIM_C_END
