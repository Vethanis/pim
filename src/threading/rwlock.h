#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "threading/event.h"

typedef struct rwlock_s
{
    i32 state;
    event_t evt;
} rwlock_t;

void rwlock_create(rwlock_t* lck);
void rwlock_destroy(rwlock_t* lck);
void rwlock_lock_read(rwlock_t* lck);
void rwlock_unlock_read(rwlock_t* lck);
void rwlock_lock_write(rwlock_t* lck);
void rwlock_unlock_write(rwlock_t* lck);

PIM_C_END
