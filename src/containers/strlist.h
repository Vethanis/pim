#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct strlist_s
{
    char** ptr;
    i32 count;
    EAlloc allocator;
} strlist_t;

void strlist_new(strlist_t* list, EAlloc allocator);
void strlist_del(strlist_t* list);
void strlist_add(strlist_t* list, const char* item);
void strlist_rm(strlist_t* list, i32 i);
i32 strlist_find(const strlist_t* list, const char* key);

PIM_C_END
