#include "threading/event.h"
#include "common/atomics.h"
#include "threading/intrin.h"

void Event_New(Event* evt)
{
    ASSERT(evt);
    Semaphore_New(&evt->sema, 0);
    store_i32(&evt->state, 0, MO_Relaxed);
}

void Event_Del(Event* evt)
{
    ASSERT(evt);
    if (evt->sema.handle)
    {
        Event_WakeAll(evt);
        Semaphore_Del(&evt->sema);
    }
}

void Event_Wait(Event* evt)
{
    ASSERT(evt);
    i32 prev = dec_i32(&evt->state, MO_AcqRel);
    if (prev < 1)
    {
        Semaphore_Wait(evt->sema);
    }
}

void Event_WakeOne(Event* evt)
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
        Intrin_Spin(++spins);
    }
    if (oldstate < 0)
    {
        Semaphore_Signal(evt->sema, 1);
    }
}

void Event_WakeAll(Event* evt)
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
        Intrin_Spin(++spins);
    }
    if (oldstate < 0)
    {
        Semaphore_Signal(evt->sema, -oldstate);
    }
}
