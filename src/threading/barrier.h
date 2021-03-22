#pragma once

#include "common/macro.h"
#include "threading/semaphore.h"

PIM_C_BEGIN

typedef struct Barrier_s
{
    Semaphore phases[2];
    i32 counter;
    i32 size;
} Barrier;

void Barrier_New(Barrier* bar, i32 size);
void Barrier_Del(Barrier* bar);

void Barrier_Phase1(Barrier* bar);
void Barrier_Phase2(Barrier* bar);

void Barrier_Wait(Barrier* bar);

PIM_C_END
