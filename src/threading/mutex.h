#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct mutex_s
{
    u64 opaque[6];
} mutex_t;

void mutex_create(mutex_t* mut);
void mutex_destroy(mutex_t* mut);
void mutex_lock(mutex_t* mut);
void mutex_unlock(mutex_t* mut);
bool mutex_trylock(mutex_t* mut);

PIM_C_END
