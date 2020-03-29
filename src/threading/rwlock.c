#include "threading/rwlock.h"
#include "common/atomics.h"
#include "threading/intrin.h"
#include <string.h>

// 0xaabbcc:
// a: readers
// b: waiting readers
// c: writers

void rwlock_create(rwlock_t* lck)
{
    ASSERT(lck);
    store_u32(&(lck->state), 0, MO_Release);
    semaphore_create(&(lck->rsema), 1);
    semaphore_create(&(lck->wsema), 1);
}

void rwlock_destroy(rwlock_t* lck)
{
    ASSERT(lck);
    semaphore_destroy(&(lck->rsema));
    semaphore_destroy(&(lck->wsema));
    memset(lck, 0, sizeof(rwlock_t));
}

void rwlock_lock_read(rwlock_t* lck)
{
    uint64_t spins = 0;
    uint32_t oldstate = 0;
    uint32_t newstate = 0;

writestate:
    oldstate = load_u32(&(lck->state), MO_Relaxed);
    uint32_t readers = 0xff & (oldstate >> 16);
    uint32_t waiters = 0xff & (oldstate >> 8);
    uint32_t writers = 0xff & (oldstate >> 0);
    if (writers)
    {
        waiters = 0xff & (waiters + 1);
        ASSERT(waiters);
    }
    else
    {
        readers = 0xff & (readers + 1);
        ASSERT(readers);
    }
    newstate = (readers << 16) | (waiters << 8) | (writers << 0);

    if (!cmpex_u32(&(lck->state), &oldstate, newstate, MO_AcqRel))
    {
        intrin_spin(++spins);
        goto writestate;
    }

    if (writers)
    {
        semaphore_wait(lck->rsema);
    }
}

void rwlock_unlock_read(rwlock_t* lck)
{
    ASSERT(lck);
    uint32_t onereader = (1 << 16) | (0 << 8) | (0 << 0);
    uint32_t oldstate = fetch_sub_u32(&(lck->state), onereader, MO_Release);
    uint32_t readers = 0xff & (oldstate >> 16);
    uint32_t waiters = 0xff & (oldstate >> 8);
    uint32_t writers = 0xff & (oldstate >> 0);
    ASSERT(readers);
    if ((readers == 1u) && (writers != 0u))
    {
        semaphore_signal(lck->wsema, 1);
    }
}

void rwlock_lock_write(rwlock_t* lck)
{
    ASSERT(lck);
    uint32_t onewriter = (0 << 16) | (0 << 8) | (1 << 0);
    uint32_t oldstate = fetch_add_u32(&(lck->state), onewriter, MO_Acquire);
    uint32_t readers = 0xff & (oldstate >> 16);
    uint32_t waiters = 0xff & (oldstate >> 8);
    uint32_t writers = 0xff & (oldstate >> 0);
    ASSERT(writers < 0xff);
    if (writers | readers)
    {
        semaphore_wait(lck->wsema);
    }
}

void rwlock_unlock_write(rwlock_t* lck)
{
    ASSERT(lck);
    uint64_t spins = 0;
    uint32_t oldstate = 0;
    uint32_t newstate = 0;

writestate:
    oldstate = load_u32(&(lck->state), MO_Relaxed);
    uint32_t readers = 0xff & (oldstate >> 16);
    uint32_t waiters = 0xff & (oldstate >> 8);
    uint32_t writers = 0xff & (oldstate >> 0);
    ASSERT(!readers);
    ASSERT(writers);
    if (waiters)
    {
        readers = waiters;
        waiters = 0;
    }
    writers = 0xff & (writers - 1u);
    if (!cmpex_u32(&(lck->state), &oldstate, newstate, MO_AcqRel))
    {
        intrin_spin(++spins);
        goto writestate;
    }
    if (waiters)
    {
        semaphore_signal(lck->rsema, waiters);
    }
    else if (writers > 1)
    {
        semaphore_signal(lck->wsema, 1);
    }
}
