#include "threading/rwlock.h"
#include "common/atomics.h"

void RwLock_New(RwLock* lck)
{
    ASSERT(lck);
    store_i32(&(lck->state), 0, MO_Release);
    Event_New(&(lck->evt));
}

void RwLock_Del(RwLock* lck)
{
    ASSERT(lck);
    store_i32(&(lck->state), 0, MO_Release);
    Event_Del(&(lck->evt));
}

void RwLock_LockReader(RwLock* lck)
{
    i32* pState = &(lck->state);
    Event* pEvent = &(lck->evt);
    i32 prev = 0;

    while (true)
    {
        prev = load_i32(pState, MO_Relaxed);
        if (prev < 0)
        {
            Event_Wait(pEvent);
        }
        if (prev >= 0 && cmpex_i32(pState, &prev, prev + 1, MO_Acquire))
        {
            return;
        }
    }
}

void RwLock_UnlockReader(RwLock* lck)
{
    i32* pState = &(lck->state);
    Event* pEvent = &(lck->evt);
    i32 prev = dec_i32(pState, MO_Release);
    ASSERT(prev >= 1);
    if (prev == 1)
    {
        Event_WakeAll(pEvent);
    }
}

void RwLock_LockWriter(RwLock* lck)
{
    i32* pState = &(lck->state);
    Event* pEvent = &(lck->evt);
    i32 prev = 0;

    while (true)
    {
        prev = load_i32(pState, MO_Relaxed);
        if (prev == 0 && cmpex_i32(pState, &prev, -1, MO_Acquire))
        {
            return;
        }
        Event_Wait(pEvent);
    }
}

void RwLock_UnlockWriter(RwLock* lck)
{
    i32* pState = &(lck->state);
    Event* pEvent = &(lck->evt);
    i32 prev = inc_i32(pState, MO_Release);
    ASSERT(prev == -1);
    Event_WakeAll(pEvent);
}
