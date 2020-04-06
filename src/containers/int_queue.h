#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct intQ_s
{
    i32* ptr;
    u32 width;
    u32 iRead;
    u32 iWrite;
    EAlloc allocator;
} intQ_t;

void intQ_create(intQ_t* q, EAlloc allocator);
void intQ_destroy(intQ_t* q);
void intQ_clear(intQ_t* q);

u32 intQ_size(const intQ_t* q);
u32 intQ_capacity(const intQ_t* q);

void intQ_reserve(intQ_t* q, u32 capacity);

void intQ_push(intQ_t* q, i32 value);
bool intQ_trypop(intQ_t* q, i32* valueOut);

PIM_C_END
