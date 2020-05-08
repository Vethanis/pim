#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "components/components.h"
#include "components/compflag.h"
#include "threading/task.h"

void ecs_sys_init(void);
void ecs_sys_update(void);
void ecs_sys_shutdown(void);

i32 ecs_ent_count(void);

ent_t ecs_create(compflag_t components, const void** data);
void ecs_destroy(ent_t entity);

bool ecs_has(ent_t entity, compid_t component);
void ecs_add(ent_t entity, compid_t component, const void* src);
void ecs_rm(ent_t entity, compid_t component);
void* ecs_get(ent_t entity, compid_t component);

bool ecs_has_all(ent_t entity, compflag_t all);
bool ecs_has_any(ent_t entity, compflag_t any);
bool ecs_has_none(ent_t entity, compflag_t none);

typedef void(PIM_CDECL *ecs_foreach_fn)(struct ecs_foreach_s* foreach, void** rows, const i32* indices, i32 length);

typedef struct ecs_foreach_s
{
    task_t task;
    ecs_foreach_fn fn;
    compflag_t all;
    compflag_t none;
} ecs_foreach_t;

void ecs_foreach(
    ecs_foreach_t* foreach,
    compflag_t all,
    compflag_t none,
    ecs_foreach_fn fn);

PIM_C_END
