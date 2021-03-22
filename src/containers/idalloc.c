#include "containers/idalloc.h"
#include "allocator/allocator.h"
#include <string.h>

void IdAlloc_New(IdAlloc* ia)
{
    memset(ia, 0, sizeof(*ia));
}

void IdAlloc_Del(IdAlloc* ia)
{
    Mem_Free(ia->versions);
    IntQueue_Del(&ia->freelist);
    memset(ia, 0, sizeof(*ia));
}

i32 IdAlloc_Capacity(const IdAlloc* ia)
{
    return ia->length;
}

i32 IdAlloc_Size(const IdAlloc* ia)
{
    return ia->length - (i32)IntQueue_Size(&ia->freelist);
}

void IdAlloc_Clear(IdAlloc* ia)
{
    ia->length = 0;
    IntQueue_Clear(&ia->freelist);
}

bool IdAlloc_Exists(const IdAlloc* ia, GenId id)
{
    i32 i = id.index;
    u8 v = id.version;
    return (i < ia->length) && (ia->versions[i] == v);
}

bool IdAlloc_ExistsAt(const IdAlloc* ia, i32 index)
{
    ASSERT((u32)index < (u32)ia->length);
    return ia->versions[index] & 1;
}

GenId IdAlloc_Alloc(IdAlloc* ia)
{
    i32 index = 0;
    if (!IntQueue_TryPop(&ia->freelist, &index))
    {
        index = ia->length++;
        Perm_Reserve(ia->versions, ia->length);
        ia->versions[index] = 0;
    }
    u8 version = ++(ia->versions[index]);
    GenId id;
    id.version = version;
    id.index = index;
    ASSERT(version & 1);
    return id;
}

bool IdAlloc_Free(IdAlloc* ia, GenId id)
{
    if (IdAlloc_Exists(ia, id))
    {
        i32 index = id.index;
        ia->versions[index]++;
        IntQueue_Push(&ia->freelist, index);
        ASSERT(!(ia->versions[index] & 1));
        return true;
    }
    return false;
}

bool IdAlloc_FreeAt(IdAlloc* ia, i32 index)
{
    if (IdAlloc_ExistsAt(ia, index))
    {
        ia->versions[index]++;
        IntQueue_Push(&ia->freelist, index);
        return true;
    }
    return false;
}
