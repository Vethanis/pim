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
#include "ui/cimgui_ext.h"
#include <string.h>

static Entities ms_drawables;
Entities *const drawables_get(void) { return &ms_drawables; }

void drawables_init(void)
{

}

void drawables_update(void)
{

}

void drawables_shutdown(void)
{
    drawables_del(drawables_get());
}

i32 drawables_add(Entities *const dr, Guid name)
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

static void DestroyAtIndex(Entities *const dr, i32 i)
{
    ASSERT(i >= 0);
    ASSERT(i < dr->count);
    mesh_release(dr->meshes[i]);
    Material* material = &dr->materials[i];
    texture_release(material->albedo);
    texture_release(material->rome);
    texture_release(material->normal);
}

static void RemoveAtIndex(Entities *const dr, i32 i)
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

bool drawables_rm(Entities *const dr, Guid name)
{
    const i32 i = drawables_find(dr, name);
    if (i == -1)
    {
        return false;
    }
    RemoveAtIndex(dr, i);
    return true;
}

i32 drawables_find(Entities const *const dr, Guid name)
{
    return guid_find(dr->names, dr->count, name);
}

void drawables_clear(Entities *const dr)
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

void drawables_del(Entities *const dr)
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
    Entities *dr;
} task_UpdateBounds;

static void UpdateBoundsFn(void *const pbase, i32 begin, i32 end)
{
    task_UpdateBounds *const task = pbase;
    Entities *const pim_noalias dr = task->dr;
    MeshId const *const pim_noalias meshes = dr->meshes;
    Box3D *const pim_noalias bounds = dr->bounds;

    for (i32 i = begin; i < end; ++i)
    {
        bounds[i] = mesh_calcbounds(meshes[i]);
    }
}

ProfileMark(pm_updatebouns, drawables_updatebounds)
void drawables_updatebounds(Entities *const dr)
{
    ProfileBegin(pm_updatebouns);

    task_UpdateBounds *const task = tmp_calloc(sizeof(*task));
    task->dr = dr;
    task_run(task, UpdateBoundsFn, dr->count);

    ProfileEnd(pm_updatebouns);
}

typedef struct task_UpdateTransforms
{
    task_t task;
    Entities* dr;
} task_UpdateTransforms;

static void UpdateTransformsFn(void* pbase, i32 begin, i32 end)
{
    task_UpdateTransforms *const task = pbase;
    Entities *const dr = task->dr;
    const float4 *const pim_noalias translations = dr->translations;
    const quat *const pim_noalias rotations = dr->rotations;
    const float4 *const pim_noalias scales = dr->scales;
    float4x4 *const pim_noalias matrices = dr->matrices;
    float3x3 *const pim_noalias invMatrices = dr->invMatrices;

    for (i32 i = begin; i < end; ++i)
    {
        matrices[i] = f4x4_trs(translations[i], rotations[i], scales[i]);
        invMatrices[i] = f3x3_IM(matrices[i]);
    }
}

ProfileMark(pm_TRS, drawables_updatetransforms)
void drawables_updatetransforms(Entities *const dr)
{
    ProfileBegin(pm_TRS);

    task_UpdateTransforms *const task = tmp_calloc(sizeof(*task));
    task->dr = dr;
    task_run(task, UpdateTransformsFn, dr->count);

    ProfileEnd(pm_TRS);
}

Box3D drawables_bounds(Entities const *const dr)
{
    const i32 length = dr->count;
    const Box3D *const pim_noalias bounds = dr->bounds;
    const float4x4 *const pim_noalias matrices = dr->matrices;

    Box3D box = box_empty();
    for (i32 i = 0; i < length; ++i)
    {
        box = box_union(box, box_transform(matrices[i], bounds[i]));
    }

    return box;
}

bool drawables_save(Crate *const crate, Entities const *const src)
{
    bool wrote = true;
    const i32 length = src->count;

    wrote &= crate_set(crate,
        guid_str("drawables.count"), &length, sizeof(length));

    // write meshes
    {
        DiskMeshId *const dmeshids = perm_calloc(sizeof(dmeshids[0]) * length);
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
        DiskMaterial *const dmaterials = perm_calloc(sizeof(dmaterials[0]) * length);
        for (i32 i = 0; i < length; ++i)
        {
            const Material* mat = &src->materials[i];
            DiskMaterial dmat = { 0 };
            dmat.flags = mat->flags;
            dmat.ior = mat->ior;
            texture_save(crate, mat->albedo, &dmat.albedo.id);
            texture_save(crate, mat->rome, &dmat.rome.id);
            texture_save(crate, mat->normal, &dmat.normal.id);
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

bool drawables_load(Crate *const crate, Entities *const dst)
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
            DiskMeshId *const dmeshids = perm_calloc(sizeof(dmeshids[0]) * len);
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
            DiskMaterial *const dmaterials = perm_calloc(sizeof(dmaterials[0]) * len);
            loaded &= crate_get(crate,
                guid_str("drawables.materials"), dmaterials, sizeof(dmaterials[0]) * len);
            if (loaded)
            {
                for (i32 i = 0; i < len; ++i)
                {
                    const DiskMaterial dmat = dmaterials[i];
                    Material mat = { 0 };
                    mat.flags = dmat.flags;
                    mat.ior = dmat.ior;
                    texture_load(crate, dmat.albedo.id, &mat.albedo);
                    texture_load(crate, dmat.rome.id, &mat.rome);
                    texture_load(crate, dmat.normal.id, &mat.normal);
                    dst->materials[i] = mat;
                    mesh_setmaterial(dst->meshes[i], &mat);
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

ProfileMark(pm_gui, drawables_gui)
void drawables_gui(bool* enabled)
{
    ProfileBegin(pm_gui);

    if (igBegin("Drawables", enabled, 0))
    {
        Entities* dr = drawables_get();
        for (i32 iDrawable = 0; iDrawable < dr->count; ++iDrawable)
        {
            igPushIDPtr(&dr->names[iDrawable]);
            char name[PIM_PATH] = { 0 };
            guid_get_name(dr->names[iDrawable], ARGS(name));

            if (igTreeNodeStr(name))
            {
                if (igTreeNodeStr("Material"))
                {
                    Material* mat = &dr->materials[iDrawable];

                    char texname[PIM_PATH];
                    texture_getnamestr(mat->albedo, ARGS(texname));
                    igText("Albedo: %s", texname);
                    texture_getnamestr(mat->rome, ARGS(texname));
                    igText("Rome: %s", texname);
                    texture_getnamestr(mat->normal, ARGS(texname));
                    igText("Normal: %s", texname);

                    bool flagBits[10];
                    char const *const flagNames[10] =
                    {
                        "Emissive",
                        "Sky",
                        "Water",
                        "Slime",
                        "Lava",
                        "Refractive",
                        "Warped",
                        "Animated",
                        "Underwater",
                        "Portal",
                    };
                    MatFlag flag = mat->flags;
                    for (u32 iBit = 0; iBit < NELEM(flagBits); ++iBit)
                    {
                        const u32 mask = 1u << iBit;
                        flagBits[iBit] = (flag & mask) != 0;
                        igCheckbox(flagNames[iBit], &flagBits[iBit]);
                        flag = (flagBits[iBit]) ? (flag | mask) : (flag & (~mask));
                    }
                    mat->flags = flag;

                    igExSliderFloat("Index of Refraction", &mat->ior, 0.01f, 10.0f);
                    igExSliderFloat("Bumpiness", &mat->bumpiness, 0.0f, 10.0f);
                    igTreePop();
                }
                igTreePop();
            }

            igPopID();
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}
