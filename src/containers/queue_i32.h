#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct queue_i32_s
{
    i32* pim_noalias ptr;
    u32 width;
    u32 iRead;
    u32 iWrite;
} queue_i32_t;

void queue_i32_new(queue_i32_t* q);
void queue_i32_del(queue_i32_t* q);
void queue_i32_clear(queue_i32_t* q);

u32 queue_i32_size(const queue_i32_t* q);
u32 queue_i32_capacity(const queue_i32_t* q);

void queue_i32_reserve(queue_i32_t* q, u32 capacity);

void queue_i32_push(queue_i32_t* q, i32 value);
bool queue_i32_trypop(queue_i32_t* q, i32* pim_noalias valueOut);

PIM_C_END
