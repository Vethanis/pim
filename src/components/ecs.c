#include "components/ecs.h"
#include "allocator/allocator.h"
#include "threading/mutex.h"
#include "threading/rwlock.h"
#include "common/atomics.h"
#include "containers/idset.h"
#include "common/profiler.h"
#include "common/pimcpy.h"

#define kSlabCapacity 1024

static const i32 kComponentSize[] =
{
    sizeof(ent_t),
    sizeof(position_t),
    sizeof(rotation_t),
    sizeof(scale_t),
    sizeof(localtoworld_t),
    sizeof(drawable_t),
    sizeof(bounds_t),
};
SASSERT(NELEM(kComponentSize) == CompId_COUNT);

typedef struct rows_s
{
    u8* rows[CompId_COUNT];
} rows_t;

typedef struct slabs_s
{
    rwlock_t lock;
    idset_t ids;
    mutex_t* locks;
    i32* lens;
    rows_t* rows;
    compflag_t* flags;
} slabs_t;

typedef struct ents_s
{
    rwlock_t lock;
    idset_t ids;
    id_t* slabs;
    i32* offsets;
} ents_t;

static slabs_t ms_slabs;
static ents_t ms_ents;

static void slabs_rlock(void) { rwlock_lock_read(&(ms_slabs.lock)); }
static void slabs_runlock(void) { rwlock_unlock_read(&(ms_slabs.lock)); }
static void slabs_wlock(void) { rwlock_lock_write(&(ms_slabs.lock)); }
static void slabs_wunlock(void) { rwlock_unlock_write(&(ms_slabs.lock)); }

static void ents_rlock(void) { rwlock_lock_read(&(ms_ents.lock)); }
static void ents_runlock(void) { rwlock_unlock_read(&(ms_ents.lock)); }
static void ents_wlock(void) { rwlock_lock_write(&(ms_ents.lock)); }
static void ents_wunlock(void) { rwlock_unlock_write(&(ms_ents.lock)); }

static void slab_lock(i32 iSlab) { mutex_lock(ms_slabs.locks + iSlab); }
static void slab_unlock(i32 iSlab) { mutex_unlock(ms_slabs.locks + iSlab); }
static void** slab_rows(i32 iSlab) { return ms_slabs.rows[iSlab].rows; }

static idset_t* slab_ids(void) { return &(ms_slabs.ids); }
static idset_t* ent_ids(void) { return &(ms_ents.ids); }

static void slabs_init(void)
{
    pimset(&ms_slabs, 0, sizeof(ms_slabs));
    rwlock_create(&(ms_slabs.lock));
    idset_create(slab_ids());
    const i32 kCapacity = 1024;
    ms_slabs.locks = pim_calloc(EAlloc_Perm, sizeof(*ms_slabs.locks) * kCapacity);
    ms_slabs.lens = pim_calloc(EAlloc_Perm, sizeof(*ms_slabs.lens) * kCapacity);
    ms_slabs.rows = pim_calloc(EAlloc_Perm, sizeof(*ms_slabs.rows) * kCapacity);
    ms_slabs.flags = pim_calloc(EAlloc_Perm, sizeof(*ms_slabs.flags) * kCapacity);
}

static void slabs_shutdown(void)
{
    slabs_wlock();
    const i32 len = ms_slabs.ids.length;
    for (i32 i = 0; i < len; ++i)
    {
        mutex_destroy(ms_slabs.locks + i);
        void** rows = slab_rows(i);
        for (i32 j = 0; j < CompId_COUNT; ++j)
        {
            pim_free(rows[j]);
            rows[j] = NULL;
        }
    }
    pim_free(ms_slabs.locks);
    ms_slabs.locks = NULL;
    pim_free(ms_slabs.lens);
    ms_slabs.lens = NULL;
    pim_free(ms_slabs.rows);
    ms_slabs.rows = NULL;
    pim_free(ms_slabs.flags);
    ms_slabs.flags = NULL;
    idset_destroy(slab_ids());
    slabs_wunlock();
    rwlock_destroy(&(ms_slabs.lock));
}

static void ents_init(void)
{
    pimset(&ms_ents, 0, sizeof(ms_ents));
    rwlock_create(&(ms_ents.lock));
    idset_create(ent_ids());
    const i32 kCapacity = 1024;
    ms_ents.slabs = pim_calloc(EAlloc_Perm, sizeof(*ms_ents.slabs) * kCapacity);
    ms_ents.offsets = pim_calloc(EAlloc_Perm, sizeof(*ms_ents.offsets) * kCapacity);
}

static void ents_shutdown(void)
{
    ents_wlock();
    pim_free(ms_ents.slabs);
    ms_ents.slabs = NULL;
    pim_free(ms_ents.offsets);
    ms_ents.offsets = NULL;
    idset_destroy(ent_ids());
    ents_wunlock();
    rwlock_destroy(&(ms_ents.lock));
}

static id_t slab_create(compflag_t flags)
{
    compflag_set(&flags, CompId_Entity);
    slabs_wlock();
    i32 len = ms_slabs.ids.length;
    const id_t id = id_alloc(slab_ids());
    const i32 iSlab = id.index;
    if (iSlab >= len)
    {
        ASSERT(iSlab == len);
        ++len;
        ms_slabs.locks = perm_realloc(ms_slabs.locks, sizeof(mutex_t) * len);
        ms_slabs.lens = perm_realloc(ms_slabs.lens, sizeof(i32) * len);
        ms_slabs.rows = perm_realloc(ms_slabs.rows, sizeof(rows_t) * len);
        ms_slabs.flags = perm_realloc(ms_slabs.flags, sizeof(compflag_t) * len);
        mutex_create(ms_slabs.locks + iSlab);
    }
    ms_slabs.lens[iSlab] = 0;
    ms_slabs.flags[iSlab] = flags;
    void** rows = slab_rows(iSlab);
    for (i32 i = 0; i < CompId_COUNT; ++i)
    {
        rows[i] = NULL;
        if (compflag_get(flags, i))
        {
            rows[i] = perm_realloc(NULL, kComponentSize[i] * kSlabCapacity);
        }
    }
    slabs_wunlock();
    return id;
}

static void slab_destroy(id_t id)
{
    slabs_wlock();
    if (id_release(slab_ids(), id))
    {
        const i32 iSlab = id.index;
        pimset(ms_slabs.flags + iSlab, 0, sizeof(compflag_t));
        ms_slabs.lens[iSlab] = 0;
        void** rows = slab_rows(iSlab);
        for (i32 i = 0; i < CompId_COUNT; ++i)
        {
            pim_free(rows[i]);
            rows[i] = NULL;
        }
    }
    slabs_wunlock();
}

static void slabset(i32 iSlab, i32 iSlot)
{
    void** rows = slab_rows(iSlab);
    for (i32 i = 0; i < CompId_COUNT; ++i)
    {
        u8* ptr = rows[i];
        const i32 stride = kComponentSize[i];
        if (ptr)
        {
            pimset(ptr + stride * iSlot, 0, stride);
        }
    }
}

static void slabcpy(i32 iSlab, i32 iSlotDst, i32 iSlotSrc)
{
    if (iSlotDst == iSlotSrc)
    {
        return;
    }
    void** rows = slab_rows(iSlab);
    for (i32 i = 0; i < CompId_COUNT; ++i)
    {
        u8* ptr = rows[i];
        const i32 stride = kComponentSize[i];
        if (ptr)
        {
            pimcpy(ptr + stride * iSlotDst, ptr + stride * iSlotSrc, stride);
        }
    }
}

static ent_t ent_create(compflag_t flags)
{
    ent_t ent;
    compflag_set(&flags, CompId_Entity);
    {
        ents_wlock();
        i32 entsLen = ms_ents.ids.length;
        const id_t entId = id_alloc(ent_ids());
        if (entId.index >= entsLen)
        {
            ASSERT(entId.index == entsLen);
            ++entsLen;
            ms_ents.offsets = perm_realloc(ms_ents.offsets, sizeof(i32) * entsLen);
            ms_ents.slabs = perm_realloc(ms_ents.slabs, sizeof(id_t) * entsLen);
        }
        ents_wunlock();
        ent.index = entId.index;
        ent.version = entId.version;
    }

    id_t slabId;
    i32 dstSlot = -1;
    slabs_rlock();
    {
        const i32 numSlabs = ms_slabs.ids.length;
        const compflag_t* slabFlags = ms_slabs.flags;
        i32* slabLens = ms_slabs.lens;
        mutex_t* slabLocks = ms_slabs.locks;

        for (i32 iSlab = numSlabs - 1; iSlab >= 0; --iSlab)
        {
            if (slabLens[iSlab] >= kSlabCapacity)
            {
                continue;
            }
            if (!compflag_eq(slabFlags[iSlab], flags))
            {
                continue;
            }
            slab_lock(iSlab);
            const i32 slabLen = slabLens[iSlab];
            if (slabLen < kSlabCapacity)
            {
                slabLens[iSlab] = slabLen + 1;
                dstSlot = slabLen;
                slabId.index = iSlab;
                slabId.version = ms_slabs.ids.versions[iSlab];
                ASSERT(slabId.version & 1);
                slabset(iSlab, dstSlot);
                ent_t* ents = (ent_t*)(slab_rows(iSlab)[CompId_Entity]);
                ents[dstSlot] = ent;
            }
            slab_unlock(iSlab);
            if (dstSlot != -1)
            {
                break;
            }
        }
    }
    slabs_runlock();

    while (dstSlot == -1)
    {
        slabId = slab_create(flags);
        const i32 iSlab = slabId.index;
        slabs_rlock();
        slab_lock(iSlab);
        const i32 slabLen = load_i32(ms_slabs.lens + iSlab, MO_Relaxed);
        if (slabLen < kSlabCapacity)
        {
            store_i32(ms_slabs.lens + iSlab, slabLen + 1, MO_Relaxed);
            dstSlot = slabLen;
            slabset(iSlab, dstSlot);
            ent_t* ents = (ent_t*)(slab_rows(iSlab)[CompId_Entity]);
            ents[dstSlot] = ent;
        }
        slab_unlock(iSlab);
        slabs_runlock();
    }

    ents_rlock();
    ms_ents.slabs[ent.index] = slabId;
    ms_ents.offsets[ent.index] = dstSlot;
    ents_runlock();

    return ent;
}

static void ent_destroy(ent_t ent)
{
    const id_t entId = { ent.index, ent.version };

    ents_wlock();
    if (id_release(ent_ids(), entId))
    {
        i32 slabBack = -1;
        const id_t slabId = ms_ents.slabs[entId.index];
        ms_ents.slabs[entId.index].version = 0;
        const i32 offset = ms_ents.offsets[entId.index];
        slabs_rlock();
        {
            ASSERT(id_current(slab_ids(), slabId));
            const i32 iSlab = slabId.index;
            slab_lock(iSlab);
            {
                slabBack = load_i32(ms_slabs.lens + iSlab, MO_Relaxed) - 1;
                store_i32(ms_slabs.lens + iSlab, slabBack, MO_Relaxed);
                ASSERT(slabBack >= 0);
                const ent_t* ents = (ent_t*)(slab_rows(iSlab)[CompId_Entity]);
                const ent_t backEnt = ents[slabBack];
                ASSERT(!pimcmp(ents + offset, &ent, sizeof(ent_t)));
                slabcpy(iSlab, offset, slabBack);
                ms_ents.offsets[backEnt.index] = offset;
            }
            slab_unlock(iSlab);
        }
        slabs_runlock();
        if (slabBack == 0)
        {
            slab_destroy(slabId);
        }
    }
    ents_wunlock();
}

static bool ent_current(ent_t ent)
{
    const id_t id = { ent.index, ent.version };
    return id_current(ent_ids(), id);
}

static compflag_t ent_flags(ent_t ent)
{
    compflag_t result;
    pimset(&result, 0, sizeof(result));
    ents_rlock();
    if (ent_current(ent))
    {
        const id_t slabId = ms_ents.slabs[ent.index];
        slabs_rlock();
        if (id_current(slab_ids(), slabId))
        {
            result = ms_slabs.flags[slabId.index];
        }
        slabs_runlock();
    }
    ents_runlock();
    return result;
}

void ecs_sys_init(void)
{
    ents_init();
    slabs_init();
}

ProfileMark(pm_update, ecs_sys_update)
void ecs_sys_update(void)
{
    ProfileBegin(pm_update);

    ProfileEnd(pm_update);
}

void ecs_sys_shutdown(void)
{
    slabs_shutdown();
    ents_shutdown();
}

i32 ecs_ent_count(void)
{
    ents_rlock();
    i32 len = ms_ents.ids.length;
    ents_runlock();
    return len;
}

i32 ecs_slab_count(void)
{
    slabs_rlock();
    i32 len = ms_slabs.ids.length;
    slabs_runlock();
    return len;
}

bool ecs_is_current(ent_t entity)
{
    ents_rlock();
    id_t id = { entity.index, entity.version };
    i32 state = id_current(ent_ids(), id);
    ents_runlock();
    return state;
}

ent_t ecs_create(compflag_t components)
{
    return ent_create(components);
}

void ecs_destroy(ent_t entity)
{
    ent_destroy(entity);
}

bool ecs_has(ent_t ent, compid_t id)
{
    return compflag_get(ent_flags(ent), id);
}

bool ecs_has_all(ent_t ent, compflag_t all)
{
    return compflag_all(ent_flags(ent), all);
}

bool ecs_has_any(ent_t ent, compflag_t any)
{
    return compflag_any(ent_flags(ent), any);
}

bool ecs_has_none(ent_t ent, compflag_t none)
{
    return compflag_none(ent_flags(ent), none);
}

ProfileMark(pm_foreach, ecs_foreach)
static void foreach_exec(task_t* task, i32 begin, i32 end)
{
    ProfileBegin(pm_foreach);

    ASSERT(task);
    ASSERT(begin >= 0);
    ASSERT(begin < end);

    ecs_foreach_t* foreach = (ecs_foreach_t*)task;
    const ecs_foreach_fn func = foreach->fn;
    const compflag_t all = foreach->all;
    const compflag_t none = foreach->none;

    slabs_rlock();
    {
        ASSERT(end <= ms_slabs.ids.length);
        for (i32 i = begin; i < end; ++i)
        {
            const compflag_t has = ms_slabs.flags[i];
            if (compflag_all(has, all) && compflag_none(has, none))
            {
                slab_lock(i);
                const i32 length = ms_slabs.lens[i];
                if (length > 0)
                {
                    func(foreach, slab_rows(i), length);
                }
                slab_unlock(i);
            }
        }
    }
    slabs_runlock();

    ProfileEnd(pm_foreach);
}

void ecs_foreach(
    ecs_foreach_t* foreach,
    compflag_t all,
    compflag_t none,
    ecs_foreach_fn fn)
{
    ASSERT(foreach);
    ASSERT(fn);
    foreach->fn = fn;
    foreach->all = all;
    foreach->none = none;
    i32 worksize = ecs_slab_count();
    if (worksize > 0)
    {
        task_submit(&(foreach->task), foreach_exec, worksize);
    }
}
