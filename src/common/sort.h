#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef i32(*CmpFn)(const void* lhs, const void* rhs, void* usr);
typedef i32(*CmpFn_i32)(i32 lhs, i32 rhs, void* usr);

void MemSwap(void* lhs, void* rhs, i32 stride);

void QuickSort(void* pVoid, i32 count, i32 stride, CmpFn cmp, void* usr);
void QuickSort_Int(i32* items, i32 count, CmpFn_i32 cmp, void* usr);
i32* IndexSort(const void* items, i32 count, i32 stride, CmpFn cmp, void* usr);

PIM_C_END
