#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct mutex_s
{
    uint64_t opaque[5];
} mutex_t;

void mutex_create(mutex_t* mut);
void mutex_destroy(mutex_t* mut);
void mutex_lock(mutex_t* mut);
void mutex_unlock(mutex_t* mut);
int32_t mutex_trylock(mutex_t* mut);

PIM_C_END
