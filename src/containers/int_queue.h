#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct intQ_s
{
    int32_t* ptr;
    uint32_t width;
    uint32_t iRead;
    uint32_t iWrite;
} intQ_t;

void intQ_create(intQ_t* q);
void intQ_destroy(intQ_t* q);
void intQ_clear(intQ_t* q);

uint32_t intQ_size(const intQ_t* q);
uint32_t intQ_capacity(const intQ_t* q);

void intQ_reserve(intQ_t* q, uint32_t capacity);

void intQ_push(intQ_t* q, int32_t value);
int32_t intQ_trypop(intQ_t* q, int32_t* valueOut);

PIM_C_END
