#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct ptrqueue_s
{
    u32 iWrite;
    u32 iRead;
    u32 width;
    isize* ptr;
    EAlloc allocator;
} ptrqueue_t;

void ptrqueue_create(ptrqueue_t *const pq, EAlloc allocator, u32 capacity);
void ptrqueue_destroy(ptrqueue_t *const pq);
u32 ptrqueue_capacity(ptrqueue_t const *const pq);
u32 ptrqueue_size(ptrqueue_t const *const pq);
bool ptrqueue_trypush(ptrqueue_t *const pq, void *const pValue);
void *const ptrqueue_trypop(ptrqueue_t *const pq);

PIM_C_END
