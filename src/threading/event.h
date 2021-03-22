#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "threading/semaphore.h"

typedef struct Event_s
{
    i32 state;
    Semaphore sema;
} Event;

void Event_New(Event* evt);
void Event_Del(Event* evt);

void Event_Wait(Event* evt);
void Event_WakeOne(Event* evt);
void Event_WakeAll(Event* evt);

PIM_C_END
