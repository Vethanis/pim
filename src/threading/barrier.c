#include "threading/barrier.h"
#include "common/atomics.h"
#include "threading/intrin.h"
#include <string.h>

void Barrier_New(Barrier* bar, i32 size)
{
    memset(bar, 0, sizeof(*bar));
    Semaphore_New(&bar->phases[0], 0);
    Semaphore_New(&bar->phases[1], 0);
    bar->size = size;
}

void Barrier_Del(Barrier* bar)
{
    if (bar)
    {
        Semaphore_Del(&bar->phases[0]);
        Semaphore_Del(&bar->phases[1]);
        memset(bar, 0, sizeof(*bar));
    }
}

void Barrier_Phase1(Barrier* bar)
{
    Semaphore phase = bar->phases[0];
    i32 size = bar->size;
    i32 counter = fetch_add_i32(&bar->counter, 1, MO_AcqRel) + 1;
    ASSERT(counter <= size);
    if (counter == size)
    {
        Semaphore_Signal(phase, size);
    }
    Semaphore_Wait(phase);
}

void Barrier_Phase2(Barrier* bar)
{
    Semaphore phase = bar->phases[1];
    i32 size = bar->size;
    i32 counter = fetch_add_i32(&bar->counter, -1, MO_AcqRel) - 1;
    ASSERT(counter >= 0);
    if (counter == 0)
    {
        Semaphore_Signal(phase, size);
    }
    Semaphore_Wait(phase);
}

void Barrier_Wait(Barrier* bar)
{
    Barrier_Phase1(bar);
    Barrier_Phase2(bar);
}
