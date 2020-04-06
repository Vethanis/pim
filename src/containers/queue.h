#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct queue_s
{
    void* ptr;
    u32 width;
    u32 iRead;
    u32 iWrite;
    u32 stride;
    EAlloc allocator;
} queue_t;

void queue_create(queue_t* queue, u32 itemSize, EAlloc allocator);
void queue_destroy(queue_t* queue);

u32 queue_size(const queue_t* queue);
u32 queue_capacity(const queue_t* queue);

void queue_clear(queue_t* queue);
void queue_reserve(queue_t* queue, u32 capacity);

void queue_push(queue_t* queue, void* src, u32 itemSize);
bool queue_trypop(queue_t* queue, void* dst, u32 itemSize);
void queue_get(queue_t* queue, u32 i, void* dst, u32 itemSize);

PIM_C_END
