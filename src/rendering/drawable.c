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
Entities *const Entities_Get(void) { return &ms_drawables; }

void EntSys_Init(void)
{

}

void EntSys_Update(void)
{

}

void EntSys_Shutdown(void)
{
    Entities_Del(Entities_Get());
}

i32 Entities_Add(Entities *const dr, Guid name)
{
    const i32 back = dr->count;
    const i32 len = back + 1;
    dr->count = len;

    Perm_Grow(dr->names, len);
    Perm_Grow(dr->meshes, len);
    Perm_Grow(dr->bounds, len);
    Perm_Grow(dr->materials, len);
    Perm_Grow(dr->matrices, len);
    Perm_Grow(dr->invMatrices, len);
    Perm_Grow(dr->translations, len);
    Perm_Grow(dr->rotations, len);
    Perm_Grow(dr->scales, len);

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
    Mesh_Release(dr->meshes[i]);
    Material* material = &dr->materials[i];
    Texture_Release(material->albedo);
    Texture_Release(material->rome);
    Texture_Release(material->normal);
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

bool Entities_Rm(Entities *const dr, Guid name)
{
    const i32 i = Entities_Find(dr, name);
    if (i == -1)
    {
        return false;
    }
    RemoveAtIndex(dr, i);
    return true;
}

i32 Entities_Find(Entities const *const dr, Guid name)
{
    return Guid_Find(dr->names, dr->count, name);
}

void Entities_Clear(Entities *const dr)
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

void Entities_Del(Entities *const dr)
{
    if (dr)
    {
        Entities_Clear(dr);
        Mem_Free(dr->names);
        Mem_Free(dr->meshes);
        Mem_Free(dr->bounds);
        Mem_Free(dr->materials);
        Mem_Free(dr->matrices);
        Mem_Free(dr->invMatrices);
        Mem_Free(dr->translations);
        Mem_Free(dr->rotations);
        Mem_Free(dr->scales);
        memset(dr, 0, sizeof(*dr));
    }
}

typedef struct task_UpdateBounds
{
    Task task;
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
        bounds[i] = Mesh_CalcBounds(meshes[i]);
    }
}

ProfileMark(pm_updatebouns, Entities_UpdateBounds)
void Entities_UpdateBounds(Entities *const dr)
{
    ProfileBegin(pm_updatebouns);

    task_UpdateBounds *const task = Temp_Calloc(sizeof(*task));
    task->dr = dr;
    Task_Run(task, UpdateBoundsFn, dr->count);

    ProfileEnd(pm_updatebouns);
}

typedef struct task_UpdateTransforms
{
    Task task;
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

ProfileMark(pm_TRS, Entities_UpdateTransforms)
void Entities_UpdateTransforms(Entities *const dr)
{
    ProfileBegin(pm_TRS);

    task_UpdateTransforms *const task = Temp_Calloc(sizeof(*task));
    task->dr = dr;
    Task_Run(task, UpdateTransformsFn, dr->count);

    ProfileEnd(pm_TRS);
}

Box3D Entities_GetBounds(Entities const *const dr)
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

bool Entities_Save(Crate *const crate, Entities const *const src)
{
    bool wrote = true;
    const i32 length = src->count;

    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.count"), &length, sizeof(length));

    // write meshes
    {
        DiskMeshId *const dmeshids = Perm_Calloc(sizeof(dmeshids[0]) * length);
        for (i32 i = 0; i < length; ++i)
        {
            Mesh_Save(crate, src->meshes[i], &dmeshids[i].id);
        }
        wrote &= Crate_Set(crate,
            Guid_FromStr("drawables.meshes"), dmeshids, sizeof(dmeshids[0]) * length);
        Mem_Free(dmeshids);
    }

    // write materials
    {
        DiskMaterial *const dmaterials = Perm_Calloc(sizeof(dmaterials[0]) * length);
        for (i32 i = 0; i < length; ++i)
        {
            const Material* mat = &src->materials[i];
            DiskMaterial dmat = { 0 };
            dmat.flags = mat->flags;
            dmat.ior = mat->ior;
            Texture_Save(crate, mat->albedo, &dmat.albedo.id);
            Texture_Save(crate, mat->rome, &dmat.rome.id);
            Texture_Save(crate, mat->normal, &dmat.normal.id);
            dmaterials[i] = dmat;
        }
        wrote &= Crate_Set(crate,
            Guid_FromStr("drawables.materials"), dmaterials, sizeof(dmaterials[0]) * length);
        Mem_Free(dmaterials);
    }

    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.names"), src->names, sizeof(src->names[0]) * length);
    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.bounds"), src->bounds, sizeof(src->bounds[0]) * length);
    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.translations"), src->translations, sizeof(src->translations[0]) * length);
    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.rotations"), src->rotations, sizeof(src->rotations[0]) * length);
    wrote &= Crate_Set(crate,
        Guid_FromStr("drawables.scales"), src->scales, sizeof(src->scales[0]) * length);

    return wrote;
}

bool Entities_Load(Crate *const crate, Entities *const dst)
{
    ASSERT(dst);
    bool loaded = false;
    Entities_Del(dst);

    i32 len = 0;
    if (Crate_Get(crate, Guid_FromStr("drawables.count"), &len, sizeof(len)) && (len > 0))
    {
        dst->count = len;
        dst->names = Perm_Calloc(sizeof(dst->names[0]) * len);
        dst->meshes = Perm_Calloc(sizeof(dst->meshes[0]) * len);
        dst->bounds = Perm_Calloc(sizeof(dst->bounds[0]) * len);
        dst->materials = Perm_Calloc(sizeof(dst->materials[0]) * len);
        dst->matrices = Perm_Calloc(sizeof(dst->matrices[0]) * len);
        dst->invMatrices = Perm_Calloc(sizeof(dst->invMatrices[0]) * len);
        dst->translations = Perm_Calloc(sizeof(dst->translations[0]) * len);
        dst->rotations = Perm_Calloc(sizeof(dst->rotations[0]) * len);
        dst->scales = Perm_Calloc(sizeof(dst->scales[0]) * len);

        loaded = true;

        // load meshes
        {
            DiskMeshId *const dmeshids = Perm_Calloc(sizeof(dmeshids[0]) * len);
            loaded &= Crate_Get(crate,
                Guid_FromStr("drawables.meshes"), dmeshids, sizeof(dmeshids[0]) * len);
            if (loaded)
            {
                for (i32 i = 0; i < len; ++i)
                {
                    Mesh_Load(crate, dmeshids[i].id, &dst->meshes[i]);
                }
            }
            Mem_Free(dmeshids);
        }

        // load materials
        {
            DiskMaterial *const dmaterials = Perm_Calloc(sizeof(dmaterials[0]) * len);
            loaded &= Crate_Get(crate,
                Guid_FromStr("drawables.materials"), dmaterials, sizeof(dmaterials[0]) * len);
            if (loaded)
            {
                for (i32 i = 0; i < len; ++i)
                {
                    const DiskMaterial dmat = dmaterials[i];
                    Material mat = { 0 };
                    mat.flags = dmat.flags;
                    mat.ior = dmat.ior;
                    Texture_Load(crate, dmat.albedo.id, &mat.albedo);
                    Texture_Load(crate, dmat.rome.id, &mat.rome);
                    Texture_Load(crate, dmat.normal.id, &mat.normal);
                    dst->materials[i] = mat;
                    Mesh_SetMaterial(dst->meshes[i], &mat);
                }
            }
            Mem_Free(dmaterials);
        }

        loaded &= Crate_Get(crate,
            Guid_FromStr("drawables.names"), dst->names, sizeof(dst->names[0]) * len);
        loaded &= Crate_Get(crate,
            Guid_FromStr("drawables.bounds"), dst->bounds, sizeof(dst->bounds[0]) * len);
        loaded &= Crate_Get(crate,
            Guid_FromStr("drawables.translations"), dst->translations, sizeof(dst->translations[0]) * len);
        loaded &= Crate_Get(crate,
            Guid_FromStr("drawables.rotations"), dst->rotations, sizeof(dst->rotations[0]) * len);
        loaded &= Crate_Get(crate,
            Guid_FromStr("drawables.scales"), dst->scales, sizeof(dst->scales[0]) * len);
    }

    return loaded;
}

ProfileMark(pm_gui, EntSys_Gui)
void EntSys_Gui(bool* enabled)
{
    ProfileBegin(pm_gui);

    if (igBegin("Drawables", enabled, 0))
    {
        Entities* dr = Entities_Get();
        for (i32 iDrawable = 0; iDrawable < dr->count; ++iDrawable)
        {
            igPushIDPtr(&dr->names[iDrawable]);
            char name[PIM_PATH] = { 0 };
            Guid_GetName(dr->names[iDrawable], ARGS(name));

            if (igTreeNodeStr(name))
            {
                if (igTreeNodeStr("Material"))
                {
                    Material* mat = &dr->materials[iDrawable];

                    char texname[PIM_PATH];
                    Texture_GetName(mat->albedo, ARGS(texname));
                    igText("Albedo: %s", texname);
                    Texture_GetName(mat->rome, ARGS(texname));
                    igText("Rome: %s", texname);
                    Texture_GetName(mat->normal, ARGS(texname));
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
