#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef enum
{
    EAlloc_Init = 0,
    EAlloc_Perm,
    EAlloc_Temp,
    EAlloc_TLS,
    EAlloc_Count
} EAlloc;

void alloc_sys_init(void);
void alloc_sys_update(void);
void alloc_sys_shutdown(void);

void* pim_malloc(EAlloc allocator, int32_t bytes);
void pim_free(void* ptr);
void* pim_realloc(EAlloc allocator, void* prev, int32_t bytes);
void* pim_calloc(EAlloc allocator, int32_t bytes);

#define pim_tmalloc(T, allocator, count) (T*)(pim_malloc(allocator, sizeof(T) * count))
#define pim_trealloc(T, allocator, prev, count) (T*)(pim_realloc(allocator, prev, sizeof(T) * count))
#define pim_tcalloc(T, allocator, count) (T*)(pim_calloc(allocator, sizeof(T) * count))

PIM_C_END
