#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct IntQueue_s
{
    i32* pim_noalias ptr;
    u32 width;
    u32 iRead;
    u32 iWrite;
} IntQueue;

void IntQueue_New(IntQueue* q);
void IntQueue_Del(IntQueue* q);
void IntQueue_Clear(IntQueue* q);

u32 IntQueue_Size(const IntQueue* q);
u32 IntQueue_Capacity(const IntQueue* q);

void IntQueue_Reserve(IntQueue* q, u32 capacity);

void IntQueue_Push(IntQueue* q, i32 value);
bool IntQueue_TryPop(IntQueue* q, i32* pim_noalias valueOut);

PIM_C_END
