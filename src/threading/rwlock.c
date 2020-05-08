#include "threading/rwlock.h"
#include "common/atomics.h"

void rwlock_create(rwlock_t* lck)
{
    ASSERT(lck);
    store_i32(&(lck->state), 0, MO_Release);
    event_create(&(lck->evt));
}

void rwlock_destroy(rwlock_t* lck)
{
    ASSERT(lck);
    store_i32(&(lck->state), 0, MO_Release);
    event_destroy(&(lck->evt));
}

void rwlock_lock_read(rwlock_t* lck)
{
    i32* pState = &(lck->state);
    event_t* pEvent = &(lck->evt);
    i32 prev = 0;

    while (true)
    {
        prev = load_i32(pState, MO_Relaxed);
        if (prev < 0)
        {
            event_wait(pEvent);
        }
        if (prev >= 0 && cmpex_i32(pState, &prev, prev + 1, MO_Acquire))
        {
            return;
        }
    }
}

void rwlock_unlock_read(rwlock_t* lck)
{
    i32* pState = &(lck->state);
    event_t* pEvent = &(lck->evt);
    i32 prev = dec_i32(pState, MO_Release);
    ASSERT(prev >= 1);
    if (prev == 1)
    {
        event_wakeall(pEvent);
    }
}

void rwlock_lock_write(rwlock_t* lck)
{
    i32* pState = &(lck->state);
    event_t* pEvent = &(lck->evt);
    i32 prev = 0;

    while (true)
    {
        prev = load_i32(pState, MO_Relaxed);
        if (prev == 0 && cmpex_i32(pState, &prev, -1, MO_Acquire))
        {
            return;
        }
        event_wait(pEvent);
    }
}

void rwlock_unlock_write(rwlock_t* lck)
{
    i32* pState = &(lck->state);
    event_t* pEvent = &(lck->evt);
    i32 prev = inc_i32(pState, MO_Release);
    ASSERT(prev == -1);
    event_wakeall(pEvent);
}
