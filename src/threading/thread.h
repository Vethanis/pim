#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    ThreadPriority_Lowest = -2,
    ThreadPriority_Lower = -1,
    ThreadPriority_Normal = 0,
    ThreadPriority_Higher = 1,
    ThreadPriority_Highest = 2,
} ThreadPriority;

typedef struct Thread_s
{
    void* handle;
} Thread;

void Thread_New(Thread* tr, i32(PIM_CDECL *entrypoint)(void*), void* data);
void Thread_Join(Thread* tr);
void Thread_SetAffinity(Thread* tr, u64 mask);
void Thread_SetPriority(Thread* tr, ThreadPriority priority);
i32 Thread_HardwareCount(void);

PIM_C_END
