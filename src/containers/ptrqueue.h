#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct PtrQueue_s
{
    u32 iWrite;
    u32 iRead;
    u32 width;
    isize* ptr;
    EAlloc allocator;
} PtrQueue;

void PtrQueue_New(PtrQueue *const pq, EAlloc allocator, u32 capacity);
void PtrQueue_Del(PtrQueue *const pq);
u32 PtrQueue_Capacity(PtrQueue const *const pq);
u32 PtrQueue_Size(PtrQueue const *const pq);
bool PtrQueue_TryPush(PtrQueue *const pq, void *const pValue);
void *const PtrQueue_TryPop(PtrQueue *const pq);

PIM_C_END
