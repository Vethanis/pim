#include "rendering/drawable.h"
#include "allocator/allocator.h"
#include "common/find.h"
#include "threading/task.h"
#include "math/float4x4_funcs.h"
#include "rendering/camera.h"
#include "math/frustum.h"
#include "rendering/constants.h"
#include "rendering/tile.h"
#include "common/profiler.h"
#include "rendering/sampler.h"
#include <string.h>

static drawables_t ms_drawables;

drawables_t* drawables_get(void) { return &ms_drawables; }

i32 drawables_add(u32 name)
{
    const i32 back = ms_drawables.count;
    const i32 len = back + 1;
    ms_drawables.count = len;

    PermGrow(ms_drawables.names, len);
    PermGrow(ms_drawables.meshes, len);
    PermGrow(ms_drawables.materials, len);
    PermGrow(ms_drawables.tmpMeshes, len);
    PermGrow(ms_drawables.bounds, len);
    PermGrow(ms_drawables.tileMasks, len);
    PermGrow(ms_drawables.matrices, len);
    PermGrow(ms_drawables.translations, len);
    PermGrow(ms_drawables.rotations, len);
    PermGrow(ms_drawables.scales, len);

    ms_drawables.names[back] = name;
    ms_drawables.translations[back] = f4_0;
    ms_drawables.scales[back] = f4_1;
    ms_drawables.rotations[back] = quat_id;
    ms_drawables.matrices[back] = f4x4_id;

    return back;
}

bool drawables_rm(u32 name)
{
    const i32 i = drawables_find(name);
    if (i == -1)
    {
        return false;
    }

    const i32 len = ms_drawables.count;
    ASSERT(len > 0);
    ASSERT(i < len);
    ms_drawables.count = len - 1;

    PopSwap(ms_drawables.names, i, len);
    PopSwap(ms_drawables.meshes, i, len);
    PopSwap(ms_drawables.materials, i, len);
    PopSwap(ms_drawables.tmpMeshes, i, len);
    PopSwap(ms_drawables.bounds, i, len);
    PopSwap(ms_drawables.tileMasks, i, len);
    PopSwap(ms_drawables.matrices, i, len);
    PopSwap(ms_drawables.translations, i, len);
    PopSwap(ms_drawables.rotations, i, len);
    PopSwap(ms_drawables.scales, i, len);

    return true;
}

i32 drawables_find(u32 name)
{
    return Find_u32(ms_drawables.names, ms_drawables.count, name);
}

typedef struct trstask_s
{
    task_t task;
} trstask_t;

pim_optimize
static void TRSFn(task_t* pBase, i32 begin, i32 end)
{
    trstask_t* pTask = (trstask_t*)pBase;
    const float4* pim_noalias translations = ms_drawables.translations;
    const quat* pim_noalias rotations = ms_drawables.rotations;
    const float4* pim_noalias scales = ms_drawables.scales;
    float4x4* pim_noalias matrices = ms_drawables.matrices;

    for (i32 i = begin; i < end; ++i)
    {
        matrices[i] = f4x4_trs(translations[i], rotations[i], scales[i]);
    }
}

ProfileMark(pm_TRS, drawables_trs)
task_t* drawables_trs(void)
{
    ProfileBegin(pm_TRS);

    trstask_t* task = tmp_calloc(sizeof(*task));
    task_submit((task_t*)task, TRSFn, ms_drawables.count);

    ProfileEnd(pm_TRS);
    return (task_t*)task;
}

typedef struct boundstask_s
{
    task_t task;
} boundstask_t;

pim_optimize
static void BoundsFn(task_t* pBase, i32 begin, i32 end)
{
    const meshid_t* pim_noalias meshes = ms_drawables.meshes;
    const float4* pim_noalias translations = ms_drawables.translations;
    const float4* pim_noalias scales = ms_drawables.scales;
    sphere_t* pim_noalias bounds = ms_drawables.bounds;
    u64* pim_noalias tileMasks = ms_drawables.tileMasks;

    mesh_t mesh;
    for (i32 i = begin; i < end; ++i)
    {
        sphere_t b = { 0 };
        u64 tilemask = 0;
        if (mesh_get(meshes[i], &mesh))
        {
            b = sphTransform(mesh.bounds, translations[i], scales[i]);
            tilemask = 1;
        }
        bounds[i] = b;
        tileMasks[i] = tilemask;
    }
}

ProfileMark(pm_Bounds, drawables_bounds)
task_t* drawables_bounds(void)
{
    ProfileBegin(pm_Bounds);

    boundstask_t* task = tmp_calloc(sizeof(*task));
    task_submit((task_t*)task, BoundsFn, ms_drawables.count);

    ProfileEnd(pm_Bounds);
    return (task_t*)task;
}

typedef struct culltask_s
{
    task_t task;
    frus_t frus;
    frus_t subfrus[kTileCount];
    plane_t fwdPlane;
    const framebuf_t* backBuf;
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
    const sphere_t* pim_noalias bounds = ms_drawables.bounds;
    u64* pim_noalias tileMasks = ms_drawables.tileMasks;

    float tileZs[kTileCount];
    for (i32 t = 0; t < kTileCount; ++t)
    {
        int2 tile = GetTile(t);
        tileZs[t] = TileDepth(tile, backBuf->depth);
    }

    for (i32 i = begin; i < end; ++i)
    {
        if (!tileMasks[i])
        {
            continue;
        }

        u64 tilemask = 0;
        const sphere_t sph = bounds[i];
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
        tileMasks[i] = tilemask;
    }
}

ProfileMark(pm_Cull, drawables_cull)
pim_optimize
task_t* drawables_cull(
    const camera_t* camera,
    const framebuf_t* backBuf)
{
    ProfileBegin(pm_Cull);
    ASSERT(camera);

    culltask_t* task = tmp_calloc(sizeof(*task));
    task->backBuf = backBuf;

    camera_frustum(camera, &(task->frus));
    for (i32 t = 0; t < kTileCount; ++t)
    {
        int2 tile = GetTile(t);
        camera_subfrustum(camera, &(task->subfrus[t]), TileMin(tile), TileMax(tile));
    }

    float4 fwd = quat_fwd(camera->rotation);
    fwd.w = f4_dot3(fwd, camera->position);
    task->fwdPlane.value = fwd;

    task_submit((task_t*)task, CullFn, ms_drawables.count);

    ProfileEnd(pm_Cull);
    return (task_t*)task;
}
