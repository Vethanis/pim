#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "threading/rwlock.h"

typedef struct ptrqueue_s
{
    rwlock_t lock;
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
void ptrqueue_clear(ptrqueue_t* pq);
void ptrqueue_reserve(ptrqueue_t* pq, u32 capacity);
void ptrqueue_push(ptrqueue_t* pq, void* pValue);
void* ptrqueue_trypop(ptrqueue_t* pq);

PIM_C_END
