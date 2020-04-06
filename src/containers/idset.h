#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "containers/int_queue.h"

typedef struct id_s
{
    i32 index;
    i32 version;
} id_t;

typedef struct idset_s
{
    i32* versions;
    i32 length;
    intQ_t queue;
} idset_t;

void idset_create(idset_t* set);
void idset_destroy(idset_t* set);

bool id_current(const idset_t* set, id_t id);

id_t id_alloc(idset_t* set);
bool id_release(idset_t* set, id_t id);

PIM_C_END
