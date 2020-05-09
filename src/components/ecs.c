#include "components/ecs.h"
#include "allocator/allocator.h"
#include "containers/queue.h"
#include "common/profiler.h"
#include <string.h>

static queue_t ms_free;
static i32 ms_length;
static i32* ms_entities;
static u32* ms_flags;
static i32* ms_versions[CompId_COUNT];
static void* ms_components[CompId_COUNT];
static const i32 kComponentSize[] =
{
    sizeof(tag_t),
    sizeof(translation_t),
    sizeof(rotation_t),
    sizeof(scale_t),
    sizeof(localtoworld_t),
    sizeof(drawable_t),
    sizeof(bounds_t),
    sizeof(light_t),
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

ent_t ecs_create(const void** data)
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
    }

    const i32 v = ++ms_entities[e];
    ms_flags[e] = 0u;

    for (i32 c = 0; c < CompId_COUNT; ++c)
    {
        const i32 stride = kComponentSize[c];
        void* dst = (u8*)ms_components[c] + stride * e;
        ms_versions[c][e] = v - 1;
        if (data && data[c])
        {
            memcpy(dst, data[c], stride);
            ms_versions[c][e] = v;
            ms_flags[e] |= (1u << c);
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
    if (ecs_current(ent))
    {
        ASSERT(ent.version & 1);
        ms_entities[ent.index] = ent.version + 1;
        ms_flags[ent.index] = 0u;
        queue_push(&ms_free, &ent.index, sizeof(ent.index));
    }
}

bool ecs_current(ent_t ent)
{
    ASSERT(ent.index < ms_length);
    return ms_entities[ent.index] == ent.version;
}

bool ecs_has(ent_t ent, compid_t id)
{
    ASSERT(ent.index < ms_length);
    return ms_versions[id][ent.index] == ent.version;
}

void ecs_add(ent_t ent, compid_t id, const void* src)
{
    ASSERT(src);
    if (ecs_current(ent))
    {
        ASSERT(ent.version & 1);
        ms_versions[id][ent.index] = ent.version;
        ms_flags[ent.index] |= (1u << id);
        const i32 stride = kComponentSize[id];
        void* dst = (u8*)ms_components[id] + ent.index * stride;
        memcpy(dst, src, stride);
    }
}

void ecs_rm(ent_t ent, compid_t id)
{
    ASSERT(ent.index < ms_length);
    if (ecs_current(ent))
    {
        ASSERT(ent.version & 1);
        ms_versions[id][ent.index] = ent.version - 1;
        ms_flags[ent.index] &= ~(1u << id);
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

bool ecs_has_all(ent_t ent, u32 all)
{
    return ecs_current(ent) && compflag_all(ms_flags[ent.index], all);
}

bool ecs_has_any(ent_t ent, u32 any)
{
    return ecs_current(ent) && compflag_any(ms_flags[ent.index], any);
}

bool ecs_has_none(ent_t ent, u32 none)
{
    return ecs_current(ent) && !ecs_has_any(ent, none);
}

ent_t* ecs_query(u32 compAll, u32 compNone, i32* countOut)
{
    ASSERT(countOut);
    ASSERT((compAll & compNone) == 0u);

    const i32 len = ms_length;
    const u32* pim_noalias flags = ms_flags;
    const i32* pim_noalias ents = ms_entities;

    ent_t* result = tmp_malloc(sizeof(result[0]) * 16);
    i32 queryLen = 0;
    for (i32 i = 0; i < len; ++i)
    {
        const i32 v = ents[i];
        const u32 has = flags[i];
        if (v & 1)
        {
            if (compflag_all(has, compAll) && compflag_none(has, compNone))
            {
                ++queryLen;
                result = tmp_realloc(result, sizeof(result[0]) * queryLen);
                result[queryLen - 1] = (ent_t){ i, v };
            }
        }
    }
    *countOut = queryLen;
    return result;
}

static void foreach_exec(task_t* task, i32 begin, i32 end)
{
    ASSERT(task);
    ASSERT(begin >= 0);
    ASSERT(begin < end);

    ecs_foreach_t* pim_noalias foreach = (ecs_foreach_t*)task;
    const ecs_foreach_fn func = foreach->fn;
    const u32 compAll = foreach->compAll;
    const u32 compNone = foreach->compNone;

    const i32 capacity = end - begin;
    i32 length = 0;
    i32* indices = tmp_malloc(sizeof(indices[0]) * capacity);

    const u32* pim_noalias flags = ms_flags;
    const i32* pim_noalias ents = ms_entities;

    for (i32 e = begin; e < end; ++e)
    {
        const i32 v = ents[e];
        const u32 has = flags[e];
        if (v & 1)
        {
            if (compflag_all(has, compAll) && compflag_none(has, compNone))
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
    u32 compAll,
    u32 compNone,
    ecs_foreach_fn fn)
{
    ASSERT(foreach);
    ASSERT(fn);
    ASSERT((compAll & compNone) == 0);

    foreach->fn = fn;
    foreach->compAll = compAll;
    foreach->compNone = compNone;
    task_submit(&(foreach->task), foreach_exec, ms_length);
}
