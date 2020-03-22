#include "threading/event.h"
#include "common/atomics.h"

void event_create(event_t* evt)
{
    ASSERT(evt);
    evt->sema = semaphore_create(0);
    store_i32(&(evt->state), 0, MO_Release);
}

void event_destroy(event_t* evt)
{
    ASSERT(evt);
    event_wakeall(evt);
    semaphore_destroy(evt->sema);
    evt->sema = NULL;
}

void event_wait(event_t* evt)
{
    ASSERT(evt);
    inc_i32(&(evt->state), MO_Acquire);
    semaphore_wait(evt->sema);
}

void event_wakeone(event_t* evt)
{
    ASSERT(evt);
    int32_t state = load_i32(&(evt->state), MO_Relaxed);
    if (state > 0)
    {
        if (cmpex_i32(&(evt->state), &state, state - 1, MO_AcqRel))
        {
            semaphore_signal(evt->sema, 1);
        }
    }
}

void event_wakeall(event_t* evt)
{
    ASSERT(evt);
    int32_t state = exch_i32(&(evt->state), 0, MO_AcqRel);
    if (state > 0)
    {
        semaphore_signal(evt->sema, state);
    }
}
