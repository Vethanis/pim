#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct arr_s
{
    void* ptr;
    i32 length;
    i32 capacity;
    i32 stride;
    EAlloc allocator;
} arr_t;

void arr_new(arr_t* arr, i32 sizeOf, EAlloc allocator);
void arr_del(arr_t* arr);

void arr_clear(arr_t* arr);
void arr_reserve(arr_t* arr, i32 minCap);
void arr_resize(arr_t* arr, i32 length);

i32 arr_len(const arr_t* arr);
void* arr_at(arr_t* arr, i32 i, i32 sizeOf);
void arr_get(const arr_t* arr, i32 i, void* dst);
void arr_set(arr_t* arr, i32 i, const void* src);

void arr_add(arr_t* arr, const void* src, i32 sizeOf);
void arr_dynadd(arr_t* arr, const void* src, i32 bytes);
void arr_pop(arr_t* arr);
void arr_remove(arr_t* arr, i32 i);

i32 arr_find(const arr_t* arr, const void* pKey, i32 sizeOf);
i32 arr_rfind(const arr_t* arr, const void* pKey, i32 sizeOf);
bool arr_findadd(arr_t* arr, const void* pKey, i32 sizeOf);
bool arr_findrm(arr_t* arr, const void* pKey, i32 sizeOf);

PIM_C_END
