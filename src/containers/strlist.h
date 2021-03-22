#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct StrList_s
{
    char** ptr;
    i32 count;
    EAlloc allocator;
} StrList;

void StrList_New(StrList* list, EAlloc allocator);
void StrList_Del(StrList* list);
void StrList_Clear(StrList* list);
void StrList_Add(StrList* list, const char* item);
void StrList_Rm(StrList* list, i32 i);
i32 StrList_Find(const StrList* list, const char* key);

PIM_C_END
