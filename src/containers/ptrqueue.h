#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "threading/rwlock.h"
#include "allocator/allocator.h"

typedef struct ptrqueue_s
{
    rwlock_t lock;
    uint32_t iWrite;
    uint32_t iRead;
    uint32_t width;
    intptr_t* ptr;
    EAlloc allocator;
} ptrqueue_t;

void ptrqueue_create(ptrqueue_t* pq, EAlloc allocator, uint32_t capacity);
void ptrqueue_destroy(ptrqueue_t* pq);
uint32_t ptrqueue_capacity(const ptrqueue_t* pq);
uint32_t ptrqueue_size(const ptrqueue_t* pq);
void ptrqueue_clear(ptrqueue_t* pq);
void ptrqueue_reserve(ptrqueue_t* pq, uint32_t capacity);
void ptrqueue_push(ptrqueue_t* pq, void* pValue);
void* ptrqueue_trypop(ptrqueue_t* pq);

PIM_C_END
