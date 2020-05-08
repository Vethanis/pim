#include "components/ecs.h"
#include "allocator/allocator.h"
#include "containers/queue.h"
#include "common/profiler.h"
#include <string.h>

static queue_t ms_free;
static i32 ms_length;
static i32* ms_entities;
static compflag_t* ms_flags;
static i32* ms_versions[CompId_COUNT];
static void* ms_components[CompId_COUNT];
static const i32 kComponentSize[] =
{
    sizeof(translation_t),
    sizeof(rotation_t),
    sizeof(scale_t),
    sizeof(localtoworld_t),
    sizeof(drawable_t),
    sizeof(bounds_t),
};
SASSERT(NELEM(kComponentSize) == CompId_COUNT);

void ecs_sys_init(void)
{
    queue_create(&ms_free, sizeof(i32), EAlloc_Perm);
    const i32 kCapacity = 256;
    ms_entities = perm_calloc(sizeof(ms_entities[0]) * kCapacity);
    ms_flags = perm_calloc(sizeof(ms_flags[0]) * kCapacity);
    for (i32 c = 0; c < CompId_COUNT; ++c)
    {
        ms_versions[c] = perm_calloc(sizeof(ms_versions[0]) * kCapacity);
        ms_components[c] = perm_calloc(kComponentSize[c] * kCapacity);
    }
    ms_length = kCapacity;
    for (i32 i = 0; i < kCapacity; ++i)
    {
        queue_push(&ms_free, &i, sizeof(i));
    }
}

void ecs_sys_update(void) {}

void ecs_sys_shutdown(void)
{
    const i32 len = ms_length;
    pim_free(ms_entities);
    ms_entities = NULL;
    pim_free(ms_flags);
    ms_flags = NULL;
    for (i32 c = 0; c < CompId_COUNT; ++c)
    {
        pim_free(ms_versions[c]);
        pim_free(ms_components[c]);
        ms_versions[c] = NULL;
        ms_components[c] = NULL;
    }
    ms_length = 0;
    queue_destroy(&ms_free);
}

i32 ecs_ent_count(void)
{
    return ms_length;
}

ent_t ecs_create(compflag_t components, const void** data)
{
    i32 e = 0;
    if (!queue_trypop(&ms_free, &e, sizeof(e)))
    {
        const i32 len = ++ms_length;
        e = len - 1;
        ms_entities = perm_realloc(ms_entities, sizeof(ms_entities[0]) * len);
        ms_flags = perm_realloc(ms_flags, sizeof(ms_flags[0]) * len);
        for (i32 c = 0; c < CompId_COUNT; ++c)
        {
            ms_versions[c] = perm_realloc(ms_versions, sizeof(ms_versions[0]) * len);
            ms_components[c] = perm_realloc(ms_components, kComponentSize[c] * len);
        }
        ms_entities[e] = 0;
        ms_flags[e] = (compflag_t) { 0 };
    }

    const i32 v = ++ms_entities[e];

    for (i32 c = 0; c < CompId_COUNT; ++c)
    {
        const i32 stride = kComponentSize[c];
        void* dst = (u8*)ms_components[c] + stride * e;
        ms_versions[c][e] = v - 1;
        if (data && data[c])
        {
            memcpy(dst, data[c], stride);
            ms_versions[c][e] = v;
            ms_flags[e].dwords[0] |= (1 << c);
        }
    }

    ent_t ent;
    ent.index = e;
    ent.version = v;
    return ent;
}

void ecs_destroy(ent_t ent)
{
    ASSERT(ent.index < ms_length);
    if (ms_entities[ent.index] == ent.version)
    {
        ASSERT(ent.version & 1);
        ms_entities[ent.index] = ent.version + 1;
        ms_flags[ent.index] = (compflag_t) { 0 };
        queue_push(&ms_free, &ent.index, sizeof(ent.index));
    }
}

bool ecs_has(ent_t ent, compid_t id)
{
    ASSERT(ent.index < ms_length);
    return ms_versions[id][ent.index] == ent.version;
}

void ecs_add(ent_t ent, compid_t id, const void* src)
{
    ASSERT(ent.index < ms_length);
    ASSERT(src);
    if (ms_entities[ent.index] == ent.version)
    {
        ASSERT(ent.version & 1);
        ms_versions[id][ent.index] = ent.version;
        ms_flags[ent.index].dwords[0] |= (1 << id);
        const i32 stride = kComponentSize[id];
        void* dst = (u8*)ms_components[id] + ent.index * stride;
        memcpy(dst, src, stride);
    }
}

void ecs_rm(ent_t ent, compid_t id)
{
    ASSERT(ent.index < ms_length);
    if (ms_entities[ent.index] == ent.version)
    {
        ASSERT(ent.version & 1);
        ms_versions[id][ent.index] = ent.version - 1;
        ms_flags[ent.index].dwords[0] &= ~(1 << id);
    }
}

void* ecs_get(ent_t ent, compid_t id)
{
    if (ecs_has(ent, id))
    {
        return (u8*)ms_components[id] + ent.index * kComponentSize[id];
    }
    return NULL;
}

bool ecs_has_all(ent_t ent, compflag_t all)
{
    ASSERT(ent.index < ms_length);
    return compflag_all(ms_flags[ent.index], all);
}

bool ecs_has_any(ent_t ent, compflag_t any)
{
    ASSERT(ent.index < ms_length);
    return compflag_any(ms_flags[ent.index], any);
}

bool ecs_has_none(ent_t ent, compflag_t none)
{
    return !ecs_has_any(ent, none);
}

static void foreach_exec(task_t* task, i32 begin, i32 end)
{
    ASSERT(task);
    ASSERT(begin >= 0);
    ASSERT(begin < end);

    ecs_foreach_t* pim_noalias foreach = (ecs_foreach_t*)task;
    const ecs_foreach_fn func = foreach->fn;
    const compflag_t all = foreach->all;
    const compflag_t none = foreach->none;

    const i32 capacity = end - begin;
    i32 length = 0;
    i32* indices = tmp_malloc(sizeof(indices[0]) * capacity);

    const i32* pim_noalias entities = ms_entities;
    const compflag_t* pim_noalias flags = ms_flags;
    for (i32 e = begin; e < end; ++e)
    {
        const i32 v = entities[e];
        const compflag_t has = flags[e];
        if (v & 1)
        {
            if (compflag_all(has, all) && compflag_none(has, none))
            {
                indices[length] = e;
                ++length;
            }
        }
    }

    func(foreach, ms_components, indices, length);
}

void ecs_foreach(
    ecs_foreach_t* foreach,
    compflag_t all,
    compflag_t none,
    ecs_foreach_fn fn)
{
    ASSERT(foreach);
    ASSERT(fn);
    ASSERT(!compflag_any(all, none));

    foreach->fn = fn;
    foreach->all = all;
    foreach->none = none;
    task_submit(&(foreach->task), foreach_exec, ms_length);
}
