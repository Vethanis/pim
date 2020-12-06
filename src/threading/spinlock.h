#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct pim_alignas(64) spinlock_s
{
    i32 state;
} spinlock_t;
SASSERT(sizeof(spinlock_t) == 64);

void spinlock_new(spinlock_t *const spin);
void spinlock_del(spinlock_t *const spin);

void spinlock_lock(spinlock_t *const spin);
void spinlock_unlock(spinlock_t *const spin);
bool spinlock_trylock(spinlock_t *const spin);

PIM_C_END
