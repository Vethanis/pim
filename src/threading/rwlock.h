#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "threading/semaphore.h"

typedef struct rwlock_s
{
    uint32_t state;
    semaphore_t rsema;
    semaphore_t wsema;
} rwlock_t;

void rwlock_create(rwlock_t* lck);
void rwlock_destroy(rwlock_t* lck);
void rwlock_lock_read(rwlock_t* lck);
void rwlock_unlock_read(rwlock_t* lck);
void rwlock_lock_write(rwlock_t* lck);
void rwlock_unlock_write(rwlock_t* lck);

PIM_C_END
