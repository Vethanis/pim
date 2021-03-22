#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "threading/event.h"

typedef struct RwLock_s
{
    i32 state;
    Event evt;
} RwLock;

void RwLock_New(RwLock* lck);
void RwLock_Del(RwLock* lck);

void RwLock_LockReader(RwLock* lck);
void RwLock_UnlockReader(RwLock* lck);

void RwLock_LockWriter(RwLock* lck);
void RwLock_UnlockWriter(RwLock* lck);

PIM_C_END
