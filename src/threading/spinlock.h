#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Spinlock_s
{
    pim_alignas(64) i32 state;
} Spinlock;
SASSERT(sizeof(Spinlock) == 64);

void Spinlock_New(Spinlock *const spin);
void Spinlock_Del(Spinlock *const spin);

void Spinlock_Lock(Spinlock *const spin);
void Spinlock_Unlock(Spinlock *const spin);
bool Spinlock_TryLock(Spinlock *const spin);

PIM_C_END
