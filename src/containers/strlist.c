#include "containers/strlist.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include <string.h>

void strlist_new(strlist_t* list, EAlloc allocator)
{
    memset(list, 0, sizeof(*list));
    list->allocator = allocator;
}

void strlist_del(strlist_t* list)
{
    strlist_clear(list);
    Mem_Free(list->ptr);
    list->ptr = NULL;
}

void strlist_clear(strlist_t* list)
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

void strlist_add(strlist_t* list, const char* item)
{
    ASSERT(item);
    i32 len = ++list->count;
    list->ptr = Mem_Realloc(list->allocator, list->ptr, sizeof(list->ptr[0]) * len);
    list->ptr[len - 1] = StrDup(item, list->allocator);
}

void strlist_rm(strlist_t* list, i32 i)
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

i32 strlist_find(const strlist_t* list, const char* key)
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
