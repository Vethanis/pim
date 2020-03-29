#include "containers/idset.h"

#include "allocator/allocator.h"

void idset_create(idset_t* set)
{
    ASSERT(set);
    set->versions = 0;
    set->length = 0;
    intQ_create(&(set->queue));
}

void idset_destroy(idset_t* set)
{
    ASSERT(set);
    pim_free(set->versions);
    set->versions = 0;
    set->length = 0;
    intQ_destroy(&(set->queue));
}

int32_t id_current(const idset_t* set, id_t id)
{
    ASSERT(set);
    ASSERT((uint32_t)id.index < (uint32_t)set->length);
    return id.version == set->versions[id.index];
}

id_t id_alloc(idset_t* set)
{
    id_t id;
    ASSERT(set);
    if (!intQ_trypop(&(set->queue), &id.index))
    {
        id.index = set->length++;
        set->versions = pim_realloc(EAlloc_Perm, set->versions, sizeof(int32_t) * set->length);
        set->versions[id.index] = 0;
    }
    id.version = ++(set->versions[id.index]);
    ASSERT(id.version & 1);
    return id;
}

int32_t id_release(idset_t* set, id_t id)
{
    if (id_current(set, id))
    {
        set->versions[id.index] += 1;
        intQ_push(&(set->queue), id.index);
        return 1;
    }
    return 0;
}
