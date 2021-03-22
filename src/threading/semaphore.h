#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Semaphore_s
{
    void* handle;
} Semaphore;

void Semaphore_New(Semaphore* sema, i32 value);
void Semaphore_Del(Semaphore* sema);
void Semaphore_Signal(Semaphore sema, i32 count);
void Semaphore_Wait(Semaphore sema);
bool Semaphore_TryWait(Semaphore sema);

PIM_C_END
