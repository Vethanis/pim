#include "threading/event.h"
#include "common/atomics.h"
#include "threading/intrin.h"

void event_create(event_t* evt)
{
    ASSERT(evt);
    semaphore_create(&evt->sema, 0);
    store_i32(&evt->state, 0, MO_Relaxed);
}

void event_destroy(event_t* evt)
{
    ASSERT(evt);
    if (evt->sema.handle)
    {
        event_wakeall(evt);
        semaphore_destroy(&evt->sema);
    }
}

void event_wait(event_t* evt)
{
    ASSERT(evt);
    i32 prev = dec_i32(&evt->state, MO_AcqRel);
    if (prev < 1)
    {
        semaphore_wait(evt->sema);
    }
}

void event_wakeone(event_t* evt)
{
    ASSERT(evt);
    u64 spins = 0;
    i32 oldstate = load_i32(&evt->state, MO_Relaxed);
    while (1)
    {
        i32 newstate = oldstate < 1 ? oldstate + 1 : 1;
        if (cmpex_i32(&evt->state, &oldstate, newstate, MO_AcqRel))
        {
            break;
        }
        intrin_spin(++spins);
    }
    if (oldstate < 0)
    {
        semaphore_signal(evt->sema, 1);
    }
}

void event_wakeall(event_t* evt)
{
    ASSERT(evt);
    u64 spins = 0;
    i32 oldstate = load_i32(&evt->state, MO_Relaxed);
    while (1)
    {
        i32 newstate = -oldstate;
        newstate = (newstate > 1) ? newstate : 1;
        if (cmpex_i32(&evt->state, &oldstate, newstate, MO_AcqRel))
        {
            break;
        }
        intrin_spin(++spins);
    }
    if (oldstate < 0)
    {
        semaphore_signal(evt->sema, -oldstate);
    }
}
