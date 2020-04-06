#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct arr_s
{
    void* ptr;
    i32 length;
    i32 capacity;
    i32 stride;
    EAlloc allocator;
} arr_t;

PIM_C_END
