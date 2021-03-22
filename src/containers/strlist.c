#include "containers/strlist.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include <string.h>

void StrList_New(StrList* list, EAlloc allocator)
{
    memset(list, 0, sizeof(*list));
    list->allocator = allocator;
}

void StrList_Del(StrList* list)
{
    StrList_Clear(list);
    Mem_Free(list->ptr);
    list->ptr = NULL;
}

void StrList_Clear(StrList* list)
{
    i32 len = list->count;
    char** pim_noalias ptr = list->ptr;
    for (i32 i = 0; i < len; ++i)
    {
        Mem_Free(ptr[i]);
        ptr[i] = NULL;
    }
    list->count = 0;
}

void StrList_Add(StrList* list, const char* item)
{
    ASSERT(item);
    i32 len = ++list->count;
    list->ptr = Mem_Realloc(list->allocator, list->ptr, sizeof(list->ptr[0]) * len);
    list->ptr[len - 1] = StrDup(item, list->allocator);
}

void StrList_Rm(StrList* list, i32 i)
{
    i32 len = list->count;
    i32 back = len - 1;
    ASSERT(i < len);
    ASSERT(back >= 0);
    list->count = back;
    char** const pim_noalias ptr = list->ptr;
    Mem_Free(ptr[i]);
    ptr[i] = ptr[back];
    ptr[back] = NULL;
}

i32 StrList_Find(const StrList* list, const char* key)
{
    ASSERT(key);
    const i32 len = list->count;
    char** const pim_noalias ptr = list->ptr;
    for (i32 i = 0; i < len; ++i)
    {
        if (StrCmp(ptr[i], PIM_PATH, key) == 0)
        {
            return i;
        }
    }
    return -1;
}
