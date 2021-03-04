#include "threading/barrier.h"
#include "common/atomics.h"
#include "threading/intrin.h"
#include <string.h>

void barrier_new(barrier_t* bar, i32 size)
{
    memset(bar, 0, sizeof(*bar));
    semaphore_create(&bar->phases[0], 0);
    semaphore_create(&bar->phases[1], 0);
    bar->size = size;
}

void barrier_del(barrier_t* bar)
{
    if (bar)
    {
        semaphore_destroy(&bar->phases[0]);
        semaphore_destroy(&bar->phases[1]);
        memset(bar, 0, sizeof(*bar));
    }
}

void barrier_phase1(barrier_t* bar)
{
    semaphore_t phase = bar->phases[0];
    i32 size = bar->size;
    i32 counter = fetch_add_i32(&bar->counter, 1, MO_AcqRel) + 1;
    ASSERT(counter <= size);
    if (counter == size)
    {
        semaphore_signal(phase, size);
    }
    semaphore_wait(phase);
}

void barrier_phase2(barrier_t* bar)
{
    semaphore_t phase = bar->phases[1];
    i32 size = bar->size;
    i32 counter = fetch_add_i32(&bar->counter, -1, MO_AcqRel) - 1;
    ASSERT(counter >= 0);
    if (counter == 0)
    {
        semaphore_signal(phase, size);
    }
    semaphore_wait(phase);
}

void barrier_wait(barrier_t* bar)
{
    barrier_phase1(bar);
    barrier_phase2(bar);
}
