#pragma once

#include "common/types.h"
#include <stdlib.h>
#include <string.h>

typedef struct arr_t { void* ptr; i32 len; i32 cap; } arr_t;
#define ArrayArgs(x) (arr_t*)&(x), sizeof((x).ptr[0])

#define ArrEmpty(x) ((x).len == 0)
#define ArrFull(x) ((x).len == (x).cap)
#define ArrByteCt(x) ( sizeof((x).ptr[0]) * (x).len )
#define ArrByteCap(x) ( sizeof((x).ptr[0]) * (x).cap )
#define ArrBack(x) ((x).ptr[(x).len - 1])

#define ArrReset(x) arr_reset(ArrayArgs(x))
inline void arr_reset(arr_t* arr, i32 stride)
{
    void* ptr = arr->ptr;
    arr->ptr=0;
    arr->len=0;
    arr->cap=0;
    if(ptr)
    {
        free(ptr);
    }
}

#define ArrReserve(x, cap) arr_reserve(ArrayArgs(x), cap)
inline void arr_reserve(arr_t* arr, i32 stride, i32 capacity)
{
    const i32 cur = arr->cap;
    const i32 next = cur * 2;
    const i32 chosen = next > capacity ? next : capacity;
    if(capacity > cur)
    {
        void* ptr = realloc(arr->ptr, stride * chosen);
        arr->ptr = ptr;
        arr->cap = chosen;
    }
}

#define ArrResize(x, len) arr_resize(ArrayArgs(x), len)
inline void arr_resize(arr_t* arr, i32 stride, i32 len)
{
    arr_reserve(arr, stride, len);
    arr->len = len;
}

#define ArrGrow(x) arr_grow(ArrayArgs(x))
inline void arr_grow(arr_t* arr, i32 stride)
{
    const i32 back = arr->len++;
    arr_reserve(arr, stride, back + 1);
}

#define ArrFind(x, key) arr_find(ArrayArgs(x), &(key))
inline i32 arr_find(arr_t* arr, i32 stride, const void* key)
{
    const u8* ptr = (const u8*)(arr->ptr);
    const i32 count = arr->len;
    for(i32 i = 0; i < count; ++i)
    {
        if(memcmp(key, ptr + stride * i, stride) == 0)
        {
            return i;
        }
    }
    return -1;
}
