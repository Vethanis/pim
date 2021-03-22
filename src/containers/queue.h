#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Queue_s
{
    void* ptr;
    u32 width;
    u32 iRead;
    u32 iWrite;
    u32 stride;
    EAlloc allocator;
} Queue;

void Queue_New(Queue* queue, u32 itemSize, EAlloc allocator);
void Queue_Del(Queue* queue);

u32 Queue_Size(const Queue* queue);
u32 Queue_Capacity(const Queue* queue);

void Queue_Clear(Queue* queue);
void Queue_Reserve(Queue* queue, u32 capacity);

void Queue_Push(Queue* queue, const void* src, u32 itemSize);
void Queue_PushFront(Queue* queue, const void* src, u32 itemSize);
bool Queue_TryPop(Queue* queue, void* dst, u32 itemSize);
void Queue_Get(Queue* queue, u32 i, void* dst, u32 itemSize);

PIM_C_END
