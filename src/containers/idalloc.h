#pragma once

#include "common/macro.h"
#include "containers/queue_i32.h"
#include "containers/genid.h"

PIM_C_BEGIN

typedef struct idalloc_s
{
    i32 length;
    u8* pim_noalias versions;
    queue_i32_t freelist;
} idalloc_t;

void idalloc_new(idalloc_t* ia);
void idalloc_del(idalloc_t* ia);
void idalloc_clear(idalloc_t* ia);

i32 idalloc_capacity(const idalloc_t* ia);
i32 idalloc_size(const idalloc_t* ia);

bool idalloc_exists(const idalloc_t* ia, genid_t id);
bool idalloc_existsat(const idalloc_t* ia, i32 index);

genid_t idalloc_alloc(idalloc_t* ia);

bool idalloc_free(idalloc_t* ia, genid_t id);
bool idalloc_freeat(idalloc_t* ia, i32 index);

PIM_C_END
