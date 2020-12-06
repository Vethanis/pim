#include "threading/spinlock.h"
#include "common/atomics.h"
#include "threading/intrin.h"

void spinlock_new(spinlock_t *const spin)
{
    store_i32(&spin->state, 0, MO_Relaxed);
}

void spinlock_del(spinlock_t *const spin)
{
    i32 state = exch_i32(&spin->state, 0, MO_Relaxed);
    ASSERT(state == 0);
}

void spinlock_lock(spinlock_t *const spin)
{
    u64 spins = 0;
    while (!spinlock_trylock(spin))
    {
        intrin_spin(++spins);
    }
}

void spinlock_unlock(spinlock_t *const spin)
{
    i32 expected = -1;
    bool released = cmpex_i32(&spin->state, &expected, 0, MO_Release);
    ASSERT(released);
}

bool spinlock_trylock(spinlock_t *const spin)
{
    i32 expected = 0;
    return cmpex_i32(&spin->state, &expected, -1, MO_Acquire);
}
