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
#include "math/sampling.h"
#include "rendering/path_tracer.h"
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
    float4 eye;
    const framebuf_t* backBuf;
} culltask_t;

SASSERT((sizeof(u64) * 8) == kTileCount);

static bool DepthCullTest(
    const float* pim_noalias depthBuf,
    int2 size,
    i32 iTile,
    float4 eye,
    sphere_t sph)
{
    const float radius = sph.value.w;
    const float4 rd = f4_normalize3(f4_sub(sph.value, eye));
    const float t = isectSphere3D((ray_t) { eye, rd }, sph);
    if (t == -1.0f)
    {
        return false;
    }
    if (t < -radius)
    {
        return false;
    }

    const int2 tile = GetTile(iTile);
    for (i32 y = 0; y < kTileHeight; ++y)
    {
        for (i32 x = 0; x < kTileWidth; ++x)
        {
            i32 sy = y + tile.y;
            i32 sx = x + tile.x;
            i32 i = sx + sy * kDrawWidth;
            if (t <= depthBuf[i])
            {
                return true;
            }
        }
    }

    return false;
}

static void CullFn(task_t* pBase, i32 begin, i32 end)
{
    culltask_t* pTask = (culltask_t*)pBase;
    const frus_t frus = pTask->frus;
    const framebuf_t* backBuf = pTask->backBuf;
    const frus_t* pim_noalias subfrus = pTask->subfrus;
    const sphere_t* pim_noalias bounds = ms_drawables.bounds;
    const float* pim_noalias depthBuf = backBuf->depth;
    const int2 bufSize = { backBuf->width, backBuf->height };
    u64* pim_noalias tileMasks = ms_drawables.tileMasks;

    camera_t camera;
    camera_get(&camera);
    float4 eye = camera.position;

    for (i32 i = begin; i < end; ++i)
    {
        if (!tileMasks[i])
        {
            continue;
        }

        u64 tilemask = 0;
        const sphere_t sphWS = bounds[i];
        float frusDist = sdFrusSph(frus, sphWS);
        if (frusDist <= 0.0f)
        {
            for (i32 iTile = 0; iTile < kTileCount; ++iTile)
            {
                float subFrusDist = sdFrusSph(subfrus[iTile], sphWS);
                if (subFrusDist <= 0.0f)
                {
                    if (DepthCullTest(depthBuf, bufSize, iTile, eye, sphWS))
                    {
                        u64 mask = 1;
                        mask <<= iTile;
                        tilemask |= mask;
                    }
                }
            }
        }
        tileMasks[i] = tilemask;
    }
}

ProfileMark(pm_Cull, drawables_cull)
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
    task->eye = camera->position;

    task_submit((task_t*)task, CullFn, ms_drawables.count);

    ProfileEnd(pm_Cull);
    return (task_t*)task;
}

typedef struct bake_s
{
    task_t task;
    const pt_scene_t* scene;
    float weight;
    i32 bounces;
} bake_t;

static void BakeFn(task_t* pbase, i32 begin, i32 end)
{
    bake_t* task = (bake_t*)pbase;
    const pt_scene_t* scene = task->scene;
    const float weight = task->weight;
    const i32 bounces = task->bounces;

    const float4x4* pim_noalias matrices = ms_drawables.matrices;
    const meshid_t* pim_noalias meshids = ms_drawables.meshes;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        mesh_t mesh;
        if (mesh_get(meshids[i], &mesh))
        {
            const float4x4 M = matrices[i];
            const float4x4 IM = f4x4_inverse(f4x4_transpose(M));

            const i32 vertCount = mesh.length;
            const float4* pim_noalias positions = mesh.positions;
            const float4* pim_noalias normals = mesh.normals;
            float4* pim_noalias bakedGI = mesh.bakedGI;

            for (i32 j = 0; (j + 3) <= vertCount; j += 3)
            {
                float4 A = f4x4_mul_pt(M, positions[j + 0]);
                float4 B = f4x4_mul_pt(M, positions[j + 1]);
                float4 C = f4x4_mul_pt(M, positions[j + 2]);

                float4 NA = f4_normalize3(f4x4_mul_dir(IM, normals[j + 0]));
                float4 NB = f4_normalize3(f4x4_mul_dir(IM, normals[j + 1]));
                float4 NC = f4_normalize3(f4x4_mul_dir(IM, normals[j + 2]));

                for (i32 k = 0; k < 3; ++k)
                {
                    float w, u, v;
                    do
                    {
                        u = prng_f32(&rng);
                        v = prng_f32(&rng);
                        w = 1.0f - (u + v);
                    } while ((u + v) > 1.0f);

                    float4 wuvt = { w, u, v, 0.0f };
                    float4 ro = f4_blend(A, B, C, wuvt);
                    float4 N = f4_normalize3(f4_blend(NA, NB, NC, wuvt));

                    float4 rd;
                    float pdf = ScatterLambertian(&rng, N, N, &rd, 1.0f);
                    ray_t ray = { ro, rd };
                    float4 light = pt_trace_ray(&rng, scene, ray, bounces);
                    light = f4_mulvs(light, pdf);

                    bakedGI[j + 0] = f4_lerpvs(bakedGI[j + 0], light, weight * w);
                    bakedGI[j + 1] = f4_lerpvs(bakedGI[j + 1], light, weight * u);
                    bakedGI[j + 2] = f4_lerpvs(bakedGI[j + 2], light, weight * v);
                }
            }
        }
    }
    prng_set(rng);
}

ProfileMark(pm_Bake, drawables_bake)
task_t* drawables_bake(const pt_scene_t* scene, float weight, i32 bounces)
{
    ProfileBegin(pm_Bake);
    ASSERT(scene);

    bake_t* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->weight = weight;
    task->bounces = bounces;
    task_submit((task_t*)task, BakeFn, ms_drawables.count);

    ProfileEnd(pm_Bake);
    return (task_t*)task;
}
