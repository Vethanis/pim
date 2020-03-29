#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "components/components.h"
#include "components/compflag.h"
#include "threading/task.h"

void ecs_sys_init(void);
void ecs_sys_update(void);
void ecs_sys_shutdown(void);

int32_t ecs_ent_count(void);
int32_t ecs_slab_count(void);

ent_t ecs_create(compflag_t components);
void ecs_destroy(ent_t entity);
int32_t ecs_is_current(ent_t entity);

int32_t ecs_has(ent_t entity, compid_t component);
int32_t ecs_has_all(ent_t entity, compflag_t all);
int32_t ecs_has_any(ent_t entity, compflag_t any);
int32_t ecs_has_none(ent_t entity, compflag_t none);

typedef void(PIM_CDECL *ecs_foreach_fn)(struct ecs_foreach_s* foreach, uint8_t** rows, int32_t length);

typedef struct ecs_foreach_s
{
    task_t task;
    ecs_foreach_fn fn;
    compflag_t all;
    compflag_t none;
} ecs_foreach_t;

SASSERT((sizeof(ecs_foreach_t) & 15) == 0);

void ecs_foreach(
    ecs_foreach_t* foreach,
    compflag_t all,
    compflag_t none,
    ecs_foreach_fn fn);

PIM_C_END
