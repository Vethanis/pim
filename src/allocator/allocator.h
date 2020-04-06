#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void alloc_sys_init(void);
void alloc_sys_update(void);
void alloc_sys_shutdown(void);

void* pim_malloc(EAlloc allocator, i32 bytes);
void pim_free(void* ptr);
void* pim_realloc(EAlloc allocator, void* prev, i32 bytes);
void* pim_calloc(EAlloc allocator, i32 bytes);

static void* perm_malloc(i32 bytes)
{
    return pim_malloc(EAlloc_Perm, bytes);
}

static void* perm_realloc(void* prev, i32 bytes)
{
    return pim_realloc(EAlloc_Perm, prev, bytes);
}

PIM_C_END
