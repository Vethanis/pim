#include "rendering/drawable.h"
#include "allocator/allocator.h"
#include "common/find.h"
#include "threading/task.h"
#include "math/float4x4_funcs.h"
#include "rendering/camera.h"
#include "math/frustum.h"
#include "math/box.h"
#include "rendering/constants.h"
#include "common/profiler.h"
#include "rendering/sampler.h"
#include "math/sampling.h"
#include "rendering/path_tracer.h"
#include "rendering/lightmap.h"
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
    PermGrow(ms_drawables.lmUvs, len);
    PermGrow(ms_drawables.matrices, len);
    PermGrow(ms_drawables.invMatrices, len);
    PermGrow(ms_drawables.translations, len);
    PermGrow(ms_drawables.rotations, len);
    PermGrow(ms_drawables.scales, len);

    ms_drawables.names[back] = name;
    ms_drawables.translations[back] = f4_0;
    ms_drawables.scales[back] = f4_1;
    ms_drawables.rotations[back] = quat_id;
    ms_drawables.matrices[back] = f4x4_id;
    ms_drawables.invMatrices[back] = f3x3_id;

    return back;
}

static void DestroyAtIndex(i32 i)
{
    ASSERT(i >= 0);
    ASSERT(i < ms_drawables.count);
    mesh_release(ms_drawables.meshes[i]);
    material_t material = ms_drawables.materials[i];
    texture_release(material.albedo);
    texture_release(material.rome);
    texture_release(material.normal);
    lm_uvs_del(ms_drawables.lmUvs + i);
}

static void RemoveAtIndex(i32 i)
{
    DestroyAtIndex(i);

    const i32 len = ms_drawables.count;
    ms_drawables.count = len - 1;
    ASSERT(len > 0);

    PopSwap(ms_drawables.names, i, len);
    PopSwap(ms_drawables.meshes, i, len);
    PopSwap(ms_drawables.materials, i, len);
    PopSwap(ms_drawables.lmUvs, i, len);
    PopSwap(ms_drawables.matrices, i, len);
    PopSwap(ms_drawables.invMatrices, i, len);
    PopSwap(ms_drawables.translations, i, len);
    PopSwap(ms_drawables.rotations, i, len);
    PopSwap(ms_drawables.scales, i, len);
}

bool drawables_rm(u32 name)
{
    const i32 i = drawables_find(name);
    if (i == -1)
    {
        return false;
    }
    RemoveAtIndex(i);
    return true;
}

i32 drawables_find(u32 name)
{
    return Find_u32(ms_drawables.names, ms_drawables.count, name);
}

void drawables_clear(void)
{
    i32 len = ms_drawables.count;
    for (i32 i = 0; i < len; ++i)
    {
        DestroyAtIndex(i);
    }
    ms_drawables.count = 0;
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
    float3x3* pim_noalias invMatrices = ms_drawables.invMatrices;

    for (i32 i = begin; i < end; ++i)
    {
        matrices[i] = f4x4_trs(translations[i], rotations[i], scales[i]);
        invMatrices[i] = f3x3_IM(matrices[i]);
    }
}

ProfileMark(pm_TRS, drawables_trs)
void drawables_trs(void)
{
    ProfileBegin(pm_TRS);

    trstask_t* task = tmp_calloc(sizeof(*task));
    task_run(&task->task, TRSFn, ms_drawables.count);

    ProfileEnd(pm_TRS);
}

box_t drawables_bounds(void)
{
    const i32 length = ms_drawables.count;
    const meshid_t* pim_noalias meshids = ms_drawables.meshes;
    const float4x4* pim_noalias matrices = ms_drawables.matrices;

    box_t box = box_empty();
    for (i32 i = 0; i < length; ++i)
    {
        mesh_t mesh;
        if (mesh_get(meshids[i], &mesh))
        {
            box = box_union(box, box_transform(matrices[i], mesh.bounds));
        }
    }

    return box;
}
