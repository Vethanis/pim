#pragma once

#include "common/macro.h"
#include "threading/semaphore.h"

PIM_C_BEGIN

typedef struct barrier_s
{
    semaphore_t phases[2];
    i32 counter;
    i32 size;
} barrier_t;

void barrier_new(barrier_t* bar, i32 size);
void barrier_del(barrier_t* bar);

void barrier_phase1(barrier_t* bar);
void barrier_phase2(barrier_t* bar);

void barrier_wait(barrier_t* bar);

PIM_C_END
