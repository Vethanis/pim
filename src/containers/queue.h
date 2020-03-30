#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include <stdbool.h>
#include "allocator/allocator.h"

typedef struct queue_s
{
    void* ptr;
    uint32_t width;
    uint32_t iRead;
    uint32_t iWrite;
    uint32_t stride;
    EAlloc allocator;
} queue_t;

void queue_create(queue_t* queue, uint32_t itemSize, EAlloc allocator);
void queue_destroy(queue_t* queue);

uint32_t queue_size(const queue_t* queue);
uint32_t queue_capacity(const queue_t* queue);

void queue_clear(queue_t* queue);
void queue_reserve(queue_t* queue, uint32_t capacity);

void queue_push(queue_t* queue, void* src, uint32_t itemSize);
bool queue_trypop(queue_t* queue, void* dst, uint32_t itemSize);
void queue_get(queue_t* queue, uint32_t i, void* dst, uint32_t itemSize);

PIM_C_END
