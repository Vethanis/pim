#include "components/drawables.h"
#include "components/table.h"
#include "components/components.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "math/float4x4_funcs.h"
#include "rendering/camera.h"
#include "math/frustum.h"
#include "rendering/constants.h"
#include "rendering/tile.h"
#include "common/profiler.h"
#include "rendering/sampler.h"

// HashStr("Drawables"); see tools/prehash.py
static const u32 drawables_hash = 3322283348u;

table_t* Drawables_New(struct tables_s* tables)
{
    table_t* table = tables_add(tables, drawables_hash);
    table_add(table, TYPE_ARGS(drawable_t));
    table_add(table, TYPE_ARGS(bounds_t));
    table_add(table, TYPE_ARGS(localtoworld_t));
    table_add(table, TYPE_ARGS(translation_t));
    table_add(table, TYPE_ARGS(rotation_t));
    table_add(table, TYPE_ARGS(scale_t));
    return table;
}

void Drawables_Del(struct tables_s* tables)
{
    table_t* table = tables_get(tables, drawables_hash);
    if (table)
    {
        const i32 len = table_width(table);
        drawable_t* drawables = table_row(table, TYPE_ARGS(drawable_t));
        if (drawables)
        {
            // release non-POD parts of table
            // TODO: ref count these shared resources
            for (i32 i = 0; i < len; ++i)
            {
                mesh_destroy(drawables[i].mesh);
                texture_destroy(drawables[i].material.albedo);
                texture_destroy(drawables[i].material.rome);
            }
        }

        tables_rm(tables, drawables_hash);
    }
}

table_t* Drawables_Get(struct tables_s* tables)
{
    return tables_get(tables, drawables_hash);
}

typedef struct trstask_s
{
    task_t task;
    const translation_t* translations;
    const rotation_t* rotations;
    const scale_t* scales;
    localtoworld_t* matrices;
} trstask_t;

pim_optimize
static void TRSFn(task_t* pBase, i32 begin, i32 end)
{
    trstask_t* pTask = (trstask_t*)pBase;
    const translation_t* pim_noalias translations = pTask->translations;
    const rotation_t* pim_noalias rotations = pTask->rotations;
    const scale_t* pim_noalias scales = pTask->scales;
    localtoworld_t* pim_noalias matrices = pTask->matrices;

    for (i32 i = begin; i < end; ++i)
    {
        matrices[i].Value = f4x4_trs(translations[i].Value, rotations[i].Value, scales[i].Value);
    }
}

ProfileMark(pm_TRS, Drawables_TRS)
task_t* Drawables_TRS(struct tables_s* tables)
{
    ProfileBegin(pm_TRS);
    task_t* result = NULL;

    table_t* table = Drawables_Get(tables);
    if (table)
    {
        trstask_t* task = tmp_calloc(sizeof(*task));
        task->translations = table_row(table, TYPE_ARGS(translation_t));
        task->rotations = table_row(table, TYPE_ARGS(rotation_t));
        task->scales = table_row(table, TYPE_ARGS(scale_t));
        task->matrices = table_row(table, TYPE_ARGS(localtoworld_t));
        task_submit((task_t*)task, TRSFn, table_width(table));
        result = (task_t*)task;
    }

    ProfileEnd(pm_TRS);
    return result;
}

typedef struct boundstask_s
{
    task_t task;
    bounds_t* bounds;
    drawable_t* drawables;
    const translation_t* translations;
    const scale_t* scales;
} boundstask_t;

pim_optimize
static void BoundsFn(task_t* pBase, i32 begin, i32 end)
{
    boundstask_t* pTask = (boundstask_t*)pBase;
    bounds_t* pim_noalias bounds = pTask->bounds;
    drawable_t* pim_noalias drawables = pTask->drawables;
    const translation_t* pim_noalias translations = pTask->translations;
    const scale_t* pim_noalias scales = pTask->scales;

    mesh_t mesh;
    for (i32 i = begin; i < end; ++i)
    {
        bounds_t b = { 0 };
        u64 tilemask = 0;
        if (mesh_get(drawables[i].mesh, &mesh))
        {
            b.Value = sphTransform(mesh.bounds, translations[i].Value, scales[i].Value);
            tilemask = 1;
        }
        bounds[i] = b;
        drawables[i].tilemask = tilemask;
    }
}

ProfileMark(pm_Bounds, Drawables_Bounds)
task_t* Drawables_Bounds(struct tables_s* tables)
{
    ProfileBegin(pm_Bounds);

    task_t* result = NULL;
    table_t* table = Drawables_Get(tables);
    if (table)
    {
        boundstask_t* task = tmp_calloc(sizeof(*task));
        task->bounds = table_row(table, TYPE_ARGS(bounds_t));
        task->drawables = table_row(table, TYPE_ARGS(drawable_t));
        task->translations = table_row(table, TYPE_ARGS(translation_t));
        task->scales = table_row(table, TYPE_ARGS(scale_t));
        task_submit((task_t*)task, BoundsFn, table_width(table));
        result = (task_t*)task;
    }

    ProfileEnd(pm_Bounds);
    return result;
}

typedef struct culltask_s
{
    task_t task;
    frus_t frus;
    frus_t subfrus[kTileCount];
    plane_t fwdPlane;
    const framebuf_t* backBuf;
    const bounds_t* bounds;
    drawable_t* drawables;
} culltask_t;

SASSERT((sizeof(u64) * 8) == kTileCount);

pim_optimize
static void CullFn(task_t* pBase, i32 begin, i32 end)
{
    culltask_t* pTask = (culltask_t*)pBase;
    const frus_t frus = pTask->frus;
    const framebuf_t* backBuf = pTask->backBuf;
    const plane_t fwdPlane = pTask->fwdPlane;
    const frus_t* pim_noalias subfrus = pTask->subfrus;
    const bounds_t* pim_noalias bounds = pTask->bounds;
    drawable_t* pim_noalias drawables = pTask->drawables;

    float tileZs[kTileCount];
    for (i32 t = 0; t < kTileCount; ++t)
    {
        int2 tile = GetTile(t);
        tileZs[t] = TileDepth(tile, backBuf->depth);
    }

    for (i32 i = begin; i < end; ++i)
    {
        if (!drawables[i].tilemask)
        {
            continue;
        }

        u64 tilemask = 0;
        const sphere_t sph = bounds[i].Value;
        float frusDist = sdFrusSph(frus, sph);
        if (frusDist <= 0.0f)
        {
            for (i32 t = 0; t < kTileCount; ++t)
            {
                float sphZ = sdPlaneSphere(fwdPlane, sph);
                if (sphZ <= tileZs[t])
                {
                    float subFrusDist = sdFrusSph(subfrus[t], sph);
                    if (subFrusDist <= 0.0f)
                    {
                        u64 mask = 1;
                        mask <<= t;
                        tilemask |= mask;
                    }
                }
            }
        }
        drawables[i].tilemask = tilemask;
    }
}

ProfileMark(pm_Cull, Drawables_Cull)
pim_optimize
task_t* Drawables_Cull(
    struct tables_s* tables,
    const struct camera_s* camera,
    const struct framebuf_s* backBuf)
{
    ProfileBegin(pm_Cull);
    ASSERT(camera);
    task_t* result = NULL;

    table_t* table = Drawables_Get(tables);
    if (table)
    {
        culltask_t* task = tmp_calloc(sizeof(*task));
        task->bounds = table_row(table, TYPE_ARGS(bounds_t));
        task->drawables = table_row(table, TYPE_ARGS(drawable_t));

        camera_frustum(camera, &(task->frus));
        for (i32 t = 0; t < kTileCount; ++t)
        {
            int2 tile = GetTile(t);
            camera_subfrustum(camera, &(task->subfrus[t]), TileMin(tile), TileMax(tile));
        }

        float4 fwd = quat_fwd(camera->rotation);
        fwd.w = f4_dot3(fwd, camera->position);
        task->fwdPlane.value = fwd;
        task->backBuf = backBuf;

        task_submit((task_t*)task, CullFn, table_width(table));
        result = (task_t*)task;
    }

    ProfileEnd(pm_Cull);
    return result;
}
