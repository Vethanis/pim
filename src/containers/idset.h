#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "containers/int_queue.h"

typedef struct id_s
{
    int32_t index;
    int32_t version;
} id_t;

typedef struct idset_s
{
    int32_t* versions;
    int32_t length;
    intQ_t queue;
} idset_t;

void idset_create(idset_t* set);
void idset_destroy(idset_t* set);

int32_t id_current(const idset_t* set, id_t id);

id_t id_alloc(idset_t* set);
int32_t id_release(idset_t* set, id_t id);

PIM_C_END
