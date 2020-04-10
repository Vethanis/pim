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

void ptrqueue_create(ptrqueue_t* pq, EAlloc allocator, u32 capacity);
void ptrqueue_destroy(ptrqueue_t* pq);
u32 ptrqueue_capacity(const ptrqueue_t* pq);
u32 ptrqueue_size(const ptrqueue_t* pq);
bool ptrqueue_trypush(ptrqueue_t* pq, void* pValue);
void* ptrqueue_trypop(ptrqueue_t* pq);

PIM_C_END
