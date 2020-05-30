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

// alternative to alloca
void* pim_pusha(i32 bytes);
void pim_popa(i32 bytes);

static void* perm_malloc(i32 bytes)
{
    return pim_malloc(EAlloc_Perm, bytes);
}

static void* perm_calloc(i32 bytes)
{
    return pim_calloc(EAlloc_Perm, bytes);
}

static void* perm_realloc(void* prev, i32 bytes)
{
    return pim_realloc(EAlloc_Perm, prev, bytes);
}

static void* tmp_malloc(i32 bytes)
{
    return pim_malloc(EAlloc_Temp, bytes);
}

static void* tmp_realloc(void* prev, i32 bytes)
{
    return pim_realloc(EAlloc_Temp, prev, bytes);
}

static void* tmp_calloc(i32 bytes)
{
    return pim_calloc(EAlloc_Temp, bytes);
}

#define ZeroElem(ptr, i)        memset((ptr) + (i), 0, sizeof((ptr)[0]))
#define PopSwap(ptr, i, len)    memcpy((ptr) + (i), (ptr) + (len) - 1, sizeof((ptr)[0]))
#define PermReserve(ptr, len)   ptr = perm_realloc((ptr), sizeof((ptr)[0]) * (len))
#define PermGrow(ptr, len)      PermReserve(ptr, len); ZeroElem(ptr, (len) - 1)

PIM_C_END
