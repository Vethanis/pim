#include "rendering/drawable.h"
#include "allocator/allocator.h"
#include "common/find.h"
#include "common/fnv1a.h"
#include "common/guid.h"
#include "common/profiler.h"
#include "math/float4x4_funcs.h"
#include "math/frustum.h"
#include "math/box.h"
#include "math/sampling.h"
#include "rendering/camera.h"
#include "rendering/path_tracer.h"
#include "rendering/lightmap.h"
#include "rendering/sampler.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "io/fstr.h"
#include "threading/task.h"
#include "assets/crate.h"
#include <string.h>

static drawables_t ms_drawables;
drawables_t* drawables_get(void) { return &ms_drawables; }

i32 drawables_add(drawables_t* dr, guid_t name)
{
    const i32 back = dr->count;
    const i32 len = back + 1;
    dr->count = len;

    PermGrow(dr->names, len);
    PermGrow(dr->meshes, len);
    PermGrow(dr->bounds, len);
    PermGrow(dr->materials, len);
    PermGrow(dr->matrices, len);
    PermGrow(dr->invMatrices, len);
    PermGrow(dr->translations, len);
    PermGrow(dr->rotations, len);
    PermGrow(dr->scales, len);

    dr->names[back] = name;
    dr->translations[back] = f4_0;
    dr->scales[back] = f4_1;
    dr->rotations[back] = quat_id;
    dr->matrices[back] = f4x4_id;
    dr->invMatrices[back] = f3x3_id;

    return back;
}

static void DestroyAtIndex(drawables_t* dr, i32 i)
{
    ASSERT(i >= 0);
    ASSERT(i < dr->count);
    mesh_release(dr->meshes[i]);
    material_t material = dr->materials[i];
    texture_release(material.albedo);
    texture_release(material.rome);
    texture_release(material.normal);
}

static void RemoveAtIndex(drawables_t* dr, i32 i)
{
    DestroyAtIndex(dr, i);

    const i32 len = dr->count;
    dr->count = len - 1;
    ASSERT(len > 0);

    PopSwap(dr->names, i, len);
    PopSwap(dr->meshes, i, len);
    PopSwap(dr->bounds, i, len);
    PopSwap(dr->materials, i, len);
    PopSwap(dr->matrices, i, len);
    PopSwap(dr->invMatrices, i, len);
    PopSwap(dr->translations, i, len);
    PopSwap(dr->rotations, i, len);
    PopSwap(dr->scales, i, len);
}

bool drawables_rm(drawables_t* dr, guid_t name)
{
    const i32 i = drawables_find(dr, name);
    if (i == -1)
    {
        return false;
    }
    RemoveAtIndex(dr, i);
    return true;
}

i32 drawables_find(const drawables_t* dr, guid_t name)
{
    return guid_find(dr->names, dr->count, name);
}

void drawables_clear(drawables_t* dr)
{
    if (dr)
    {
        const i32 len = dr->count;
        for (i32 i = 0; i < len; ++i)
        {
            DestroyAtIndex(dr, i);
        }
        dr->count = 0;
    }
}

void drawables_del(drawables_t* dr)
{
    if (dr)
    {
        drawables_clear(dr);
        pim_free(dr->names);
        pim_free(dr->meshes);
        pim_free(dr->bounds);
        pim_free(dr->materials);
        pim_free(dr->matrices);
        pim_free(dr->invMatrices);
        pim_free(dr->translations);
        pim_free(dr->rotations);
        pim_free(dr->scales);
        memset(dr, 0, sizeof(*dr));
    }
}

typedef struct task_UpdateBounds
{
    task_t task;
    drawables_t* dr;
} task_UpdateBounds;

static void UpdateBoundsFn(task_t* pbase, i32 begin, i32 end)
{
    task_UpdateBounds* task = (task_UpdateBounds*)pbase;
    drawables_t* dr = task->dr;
    const meshid_t* pim_noalias meshes = dr->meshes;
    box_t* pim_noalias bounds = dr->bounds;

    for (i32 i = begin; i < end; ++i)
    {
        bounds[i] = mesh_calcbounds(meshes[i]);
    }
}

ProfileMark(pm_updatebouns, drawables_updatebounds)
void drawables_updatebounds(drawables_t* dr)
{
    ProfileBegin(pm_updatebouns);

    task_UpdateBounds* task = tmp_calloc(sizeof(*task));
    task->dr = dr;
    task_run(&task->task, UpdateBoundsFn, dr->count);

    ProfileEnd(pm_updatebouns);
}

typedef struct task_UpdateTransforms
{
    task_t task;
    drawables_t* dr;
} task_UpdateTransforms;

static void UpdateTransformsFn(task_t* pbase, i32 begin, i32 end)
{
    task_UpdateTransforms* task = (task_UpdateTransforms*)pbase;
    drawables_t* dr = task->dr;
    const float4* pim_noalias translations = dr->translations;
    const quat* pim_noalias rotations = dr->rotations;
    const float4* pim_noalias scales = dr->scales;
    float4x4* pim_noalias matrices = dr->matrices;
    float3x3* pim_noalias invMatrices = dr->invMatrices;

    for (i32 i = begin; i < end; ++i)
    {
        matrices[i] = f4x4_trs(translations[i], rotations[i], scales[i]);
        invMatrices[i] = f3x3_IM(matrices[i]);
    }
}

ProfileMark(pm_TRS, drawables_updatetransforms)
void drawables_updatetransforms(drawables_t* dr)
{
    ProfileBegin(pm_TRS);

    task_UpdateTransforms* task = tmp_calloc(sizeof(*task));
    task->dr = dr;
    task_run(&task->task, UpdateTransformsFn, dr->count);

    ProfileEnd(pm_TRS);
}

box_t drawables_bounds(const drawables_t* dr)
{
    const i32 length = dr->count;
    const box_t* pim_noalias bounds = dr->bounds;
    const float4x4* pim_noalias matrices = dr->matrices;

    box_t box = box_empty();
    for (i32 i = 0; i < length; ++i)
    {
        box = box_union(box, box_transform(matrices[i], bounds[i]));
    }

    return box;
}

bool drawables_save(crate_t* crate, const drawables_t* src)
{
    bool wrote = true;
    const i32 length = src->count;

    wrote &= crate_set(crate,
        guid_str("drawables.count"), &length, sizeof(length));

    // write meshes
    {
        dmeshid_t* dmeshids = perm_calloc(sizeof(dmeshids[0]) * length);
        for (i32 i = 0; i < length; ++i)
        {
            mesh_save(crate, src->meshes[i], &dmeshids[i].id);
        }
        wrote &= crate_set(crate,
            guid_str("drawables.meshes"), dmeshids, sizeof(dmeshids[0]) * length);
        pim_free(dmeshids);
    }

    // write materials
    {
        dmaterial_t* dmaterials = perm_calloc(sizeof(dmaterials[0]) * length);
        for (i32 i = 0; i < length; ++i)
        {
            const material_t mat = src->materials[i];
            dmaterial_t dmat = { 0 };
            dmat.flatAlbedo = mat.flatAlbedo;
            dmat.flatRome = mat.flatRome;
            dmat.flags = mat.flags;
            dmat.ior = mat.ior;
            texture_save(crate, mat.albedo, &dmat.albedo.id);
            texture_save(crate, mat.rome, &dmat.rome.id);
            texture_save(crate, mat.normal, &dmat.normal.id);
            dmaterials[i] = dmat;
        }
        wrote &= crate_set(crate,
            guid_str("drawables.materials"), dmaterials, sizeof(dmaterials[0]) * length);
        pim_free(dmaterials);
    }

    wrote &= crate_set(crate,
        guid_str("drawables.names"), src->names, sizeof(src->names[0]) * length);
    wrote &= crate_set(crate,
        guid_str("drawables.bounds"), src->bounds, sizeof(src->bounds[0]) * length);
    wrote &= crate_set(crate,
        guid_str("drawables.translations"), src->translations, sizeof(src->translations[0]) * length);
    wrote &= crate_set(crate,
        guid_str("drawables.rotations"), src->rotations, sizeof(src->rotations[0]) * length);
    wrote &= crate_set(crate,
        guid_str("drawables.scales"), src->scales, sizeof(src->scales[0]) * length);

    return wrote;
}

bool drawables_load(crate_t* crate, drawables_t* dst)
{
    ASSERT(dst);
    bool loaded = false;
    drawables_del(dst);

    i32 len = 0;
    if (crate_get(crate, guid_str("drawables.count"), &len, sizeof(len)) && (len > 0))
    {
        dst->count = len;
        dst->names = perm_calloc(sizeof(dst->names[0]) * len);
        dst->meshes = perm_calloc(sizeof(dst->meshes[0]) * len);
        dst->bounds = perm_calloc(sizeof(dst->bounds[0]) * len);
        dst->materials = perm_calloc(sizeof(dst->materials[0]) * len);
        dst->matrices = perm_calloc(sizeof(dst->matrices[0]) * len);
        dst->invMatrices = perm_calloc(sizeof(dst->invMatrices[0]) * len);
        dst->translations = perm_calloc(sizeof(dst->translations[0]) * len);
        dst->rotations = perm_calloc(sizeof(dst->rotations[0]) * len);
        dst->scales = perm_calloc(sizeof(dst->scales[0]) * len);

        loaded = true;

        // load meshes
        {
            dmeshid_t* dmeshids = perm_calloc(sizeof(dmeshids[0]) * len);
            loaded &= crate_get(crate,
                guid_str("drawables.meshes"), dmeshids, sizeof(dmeshids[0]) * len);
            if (loaded)
            {
                for (i32 i = 0; i < len; ++i)
                {
                    mesh_load(crate, dmeshids[i].id, &dst->meshes[i]);
                }
            }
            pim_free(dmeshids);
        }

        // load materials
        {
            dmaterial_t* dmaterials = perm_calloc(sizeof(dmaterials[0]) * len);
            loaded &= crate_get(crate,
                guid_str("drawables.materials"), dmaterials, sizeof(dmaterials[0]) * len);
            if (loaded)
            {
                for (i32 i = 0; i < len; ++i)
                {
                    const dmaterial_t dmat = dmaterials[i];
                    material_t mat = { 0 };
                    mat.flatAlbedo = dmat.flatAlbedo;
                    mat.flatRome = dmat.flatRome;
                    mat.flags = dmat.flags;
                    mat.ior = dmat.ior;
                    texture_load(crate, dmat.albedo.id, &mat.albedo);
                    texture_load(crate, dmat.rome.id, &mat.rome);
                    texture_load(crate, dmat.normal.id, &mat.normal);
                    dst->materials[i] = mat;
                }
            }
            pim_free(dmaterials);
        }

        loaded &= crate_get(crate,
            guid_str("drawables.names"), dst->names, sizeof(dst->names[0]) * len);
        loaded &= crate_get(crate,
            guid_str("drawables.bounds"), dst->bounds, sizeof(dst->bounds[0]) * len);
        loaded &= crate_get(crate,
            guid_str("drawables.translations"), dst->translations, sizeof(dst->translations[0]) * len);
        loaded &= crate_get(crate,
            guid_str("drawables.rotations"), dst->rotations, sizeof(dst->rotations[0]) * len);
        loaded &= crate_get(crate,
            guid_str("drawables.scales"), dst->scales, sizeof(dst->scales[0]) * len);
    }

    return loaded;
}
