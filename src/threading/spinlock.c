#include "threading/spinlock.h"
#include "common/atomics.h"
#include "threading/intrin.h"

void Spinlock_New(Spinlock *const spin)
{
    store_i32(&spin->state, 0, MO_Relaxed);
}

void Spinlock_Del(Spinlock *const spin)
{
    i32 state = exch_i32(&spin->state, 0, MO_Relaxed);
    ASSERT(state == 0);
}

void Spinlock_Lock(Spinlock *const spin)
{
    u64 spins = 0;
    while (!Spinlock_TryLock(spin))
    {
        Intrin_Spin(++spins);
    }
}

void Spinlock_Unlock(Spinlock *const spin)
{
    i32 expected = -1;
    bool released = cmpex_i32(&spin->state, &expected, 0, MO_Release);
    ASSERT(released);
}

bool Spinlock_TryLock(Spinlock *const spin)
{
    i32 expected = 0;
    return cmpex_i32(&spin->state, &expected, -1, MO_Acquire);
}
