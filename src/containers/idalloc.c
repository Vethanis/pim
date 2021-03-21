#include "containers/idalloc.h"
#include "allocator/allocator.h"
#include <string.h>

void idalloc_new(idalloc_t* ia)
{
    memset(ia, 0, sizeof(*ia));
}

void idalloc_del(idalloc_t* ia)
{
    Mem_Free(ia->versions);
    queue_i32_del(&ia->freelist);
    memset(ia, 0, sizeof(*ia));
}

i32 idalloc_capacity(const idalloc_t* ia)
{
    return ia->length;
}

i32 idalloc_size(const idalloc_t* ia)
{
    return ia->length - (i32)queue_i32_size(&ia->freelist);
}

void idalloc_clear(idalloc_t* ia)
{
    ia->length = 0;
    queue_i32_clear(&ia->freelist);
}

bool idalloc_exists(const idalloc_t* ia, GenId id)
{
    i32 i = id.index;
    u8 v = id.version;
    return (i < ia->length) && (ia->versions[i] == v);
}

bool idalloc_existsat(const idalloc_t* ia, i32 index)
{
    ASSERT((u32)index < (u32)ia->length);
    return ia->versions[index] & 1;
}

GenId idalloc_alloc(idalloc_t* ia)
{
    i32 index = 0;
    if (!queue_i32_trypop(&ia->freelist, &index))
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

bool idalloc_free(idalloc_t* ia, GenId id)
{
    if (idalloc_exists(ia, id))
    {
        i32 index = id.index;
        ia->versions[index]++;
        queue_i32_push(&ia->freelist, index);
        ASSERT(!(ia->versions[index] & 1));
        return true;
    }
    return false;
}

bool idalloc_freeat(idalloc_t* ia, i32 index)
{
    if (idalloc_existsat(ia, index))
    {
        ia->versions[index]++;
        queue_i32_push(&ia->freelist, index);
        return true;
    }
    return false;
}
