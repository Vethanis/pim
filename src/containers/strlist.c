#include "containers/strlist.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include <string.h>

void strlist_new(strlist_t* list, EAlloc allocator)
{
    ASSERT(list);
    memset(list, 0, sizeof(*list));
    list->allocator = allocator;
}

void strlist_del(strlist_t* list)
{
    ASSERT(list);
    u32 len = list->count;
    char** pim_noalias ptr = list->ptr;
    for (u32 i = 0; i < len; ++i)
    {
        pim_free(ptr[i]);
        ptr[i] = NULL;
    }
    list->ptr = NULL;
    list->count = 0;
}

void strlist_add(strlist_t* list, const char* item)
{
    ASSERT(list);
    ASSERT(item);
    u32 len = ++(list->count);
    PermReserve(list->ptr, len);
    list->ptr[len - 1] = StrDup(item, list->allocator);
}

void strlist_rm(strlist_t* list, i32 i)
{
    ASSERT(list);
    u32 len = list->count;
    u32 back = len - 1u;
    ASSERT((u32)i < len);
    ASSERT(back >= 0);
    list->count = back;
    char** pim_noalias ptr = list->ptr;
    pim_free(ptr[i]);
    ptr[i] = ptr[back];
    ptr[back] = NULL;
}

i32 strlist_find(const strlist_t* list, const char* key)
{
    ASSERT(list);
    ASSERT(key);
    const u32 len = list->count;
    const char** pim_noalias ptr = list->ptr;
    for (u32 i = 0; i < len; ++i)
    {
        if (StrCmp(ptr[i], PIM_PATH, key) == 0)
        {
            return (i32)i;
        }
    }
    return -1;
}
