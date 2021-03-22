#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Mutex_s
{
    u64 opaque[6];
} Mutex;

void Mutex_New(Mutex* mut);
void Mutex_Del(Mutex* mut);
void Mutex_Lock(Mutex* mut);
void Mutex_Unlock(Mutex* mut);
bool Mutex_TryLock(Mutex* mut);

PIM_C_END
