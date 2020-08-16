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
#include "rendering/constants.h"
#include "rendering/path_tracer.h"
#include "rendering/lightmap.h"
#include "rendering/sampler.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "io/fstr.h"
#include "threading/task.h"
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
    PermGrow(dr->materials, len);
    PermGrow(dr->lmUvs, len);
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
    lm_uvs_del(dr->lmUvs + i);
}

static void RemoveAtIndex(drawables_t* dr, i32 i)
{
    DestroyAtIndex(dr, i);

    const i32 len = dr->count;
    dr->count = len - 1;
    ASSERT(len > 0);

    PopSwap(dr->names, i, len);
    PopSwap(dr->meshes, i, len);
    PopSwap(dr->materials, i, len);
    PopSwap(dr->lmUvs, i, len);
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
        pim_free(dr->materials);
        pim_free(dr->lmUvs);
        pim_free(dr->matrices);
        pim_free(dr->invMatrices);
        pim_free(dr->translations);
        pim_free(dr->rotations);
        pim_free(dr->scales);
        memset(dr, 0, sizeof(*dr));
    }
}

typedef struct trstask_s
{
    task_t task;
    drawables_t* dr;
} trstask_t;

static void TRSFn(task_t* pbase, i32 begin, i32 end)
{
    trstask_t* task = (trstask_t*)pbase;
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

ProfileMark(pm_TRS, drawables_trs)
void drawables_trs(drawables_t* dr)
{
    ProfileBegin(pm_TRS);

    trstask_t* task = tmp_calloc(sizeof(*task));
    task->dr = dr;
    task_run(&task->task, TRSFn, dr->count);

    ProfileEnd(pm_TRS);
}

box_t drawables_bounds(const drawables_t* dr)
{
    const i32 length = dr->count;
    const meshid_t* pim_noalias meshids = dr->meshes;
    const float4x4* pim_noalias matrices = dr->matrices;

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

bool drawables_save(const drawables_t* src, guid_t name)
{
    char filename[PIM_PATH] = "data/";
    guid_tofile(ARGS(filename), name, ".drawables");

    fstr_t fd = fstr_open(filename, "wb");
    if (fstr_isopen(fd))
    {
        void* scratch = NULL;
        const i32 length = src->count;
        i32 offset = 0;

        // write header
        ddrawables_t hdr = { 0 };
        dbytes_new(1, sizeof(hdr), &offset);
        hdr.version = kDrawablesVersion;
        hdr.length = length;
        hdr.names = dbytes_new(length, sizeof(src->names[0]), &offset);
        hdr.meshes = dbytes_new(length, sizeof(dmeshid_t), &offset);
        hdr.materials = dbytes_new(length, sizeof(dmaterial_t), &offset);
        hdr.lmuvs = dbytes_new(length, sizeof(dlm_uvs_t), &offset);
        hdr.translations = dbytes_new(length, sizeof(src->translations[0]), &offset);
        hdr.rotations = dbytes_new(length, sizeof(src->rotations[0]), &offset);
        hdr.scales = dbytes_new(length, sizeof(src->scales[0]), &offset);
        hdr.lmpack = name;
        ASSERT(fstr_tell(fd) == 0);
        fstr_write(fd, &hdr, sizeof(hdr));

        // write names
        ASSERT(fstr_tell(fd) == hdr.names.offset);
        fstr_write(fd, src->names, sizeof(src->names[0]) * length);

        // write meshes
        {
            dmeshid_t* dmeshids = tmp_realloc(scratch, sizeof(dmeshids[0]) * length);
            for (i32 i = 0; i < length; ++i)
            {
                mesh_save(src->meshes[i], &dmeshids[i].id);
            }
            ASSERT(fstr_tell(fd) == hdr.meshes.offset);
            fstr_write(fd, dmeshids, sizeof(dmeshids[0]) * length);
            scratch = dmeshids;
        }
        // write materials
        {
            dmaterial_t* dmaterials = tmp_realloc(scratch, sizeof(dmaterials[0]) * length);
            for (i32 i = 0; i < length; ++i)
            {
                const material_t mat = src->materials[i];
                dmaterial_t dmat = { 0 };
                dmat.st = mat.st;
                dmat.flatAlbedo = mat.flatAlbedo;
                dmat.flatRome = mat.flatRome;
                dmat.flags = mat.flags;
                dmat.ior = mat.ior;
                texture_save(mat.albedo, &dmat.albedo.id);
                texture_save(mat.rome, &dmat.rome.id);
                texture_save(mat.normal, &dmat.normal.id);
                dmaterials[i] = dmat;
            }
            ASSERT(fstr_tell(fd) == hdr.materials.offset);
            fstr_write(fd, dmaterials, sizeof(dmaterials[0]) * length);
            scratch = dmaterials;
        }
        // write lightmap uv headers
        dlm_uvs_t* dlmuvs = tmp_realloc(scratch, sizeof(dlmuvs[0]) * length);
        {
            for (i32 i = 0; i < length; ++i)
            {
                const lm_uvs_t lmuv = src->lmUvs[i];
                dlm_uvs_t dlmuv = { 0 };
                dlmuv.length = lmuv.length;
                dlmuv.uvs = dbytes_new(lmuv.length, sizeof(lmuv.uvs[0]), &offset);
                dlmuv.indices = dbytes_new(lmuv.length, sizeof(lmuv.indices[0]), &offset);
                dlmuvs[i] = dlmuv;
            }
            ASSERT(fstr_tell(fd) == hdr.lmuvs.offset);
            fstr_write(fd, dlmuvs, sizeof(dlmuvs[0]) * length);
            scratch = NULL;
        }

        // write translations
        ASSERT(fstr_tell(fd) == hdr.translations.offset);
        fstr_write(fd, src->translations, sizeof(src->translations[0]) * length);

        // write rotations
        ASSERT(fstr_tell(fd) == hdr.rotations.offset);
        fstr_write(fd, src->rotations, sizeof(src->rotations[0]) * length);

        // write scales
        ASSERT(fstr_tell(fd) == hdr.scales.offset);
        fstr_write(fd, src->scales, sizeof(src->scales[0]) * length);

        // write lightmap uv contents
        for (i32 i = 0; i < length; ++i)
        {
            const lm_uvs_t lmuv = src->lmUvs[i];
            ASSERT(lmuv.length == dlmuvs[i].length);
            ASSERT(dlmuvs[i].uvs.size == sizeof(lmuv.uvs[0]) * lmuv.length);
            ASSERT(dlmuvs[i].indices.size == sizeof(lmuv.indices[0]) * lmuv.length);

            ASSERT(fstr_tell(fd) == dlmuvs[i].uvs.offset);
            fstr_write(fd, lmuv.uvs, sizeof(lmuv.uvs[0]) * lmuv.length);

            ASSERT(fstr_tell(fd) == dlmuvs[i].indices.offset);
            fstr_write(fd, lmuv.indices, sizeof(lmuv.indices[0]) * lmuv.length);
        }

        fstr_close(&fd);
        return true;
    }
    return false;
}

bool drawables_load(drawables_t* dst, guid_t name)
{
    ASSERT(dst);
    bool loaded = false;
    drawables_del(dst);

    char filename[PIM_PATH] = "data/";
    guid_tofile(ARGS(filename), name, ".drawables");

    fstr_t fd = fstr_open(filename, "rb");
    if (fstr_isopen(fd))
    {
        ddrawables_t hdr = { 0 };
        fstr_read(fd, &hdr, sizeof(hdr));
        if (hdr.version == kDrawablesVersion)
        {
            const i32 len = hdr.length;
            void* scratch = NULL;
            if (len > 0)
            {
                dbytes_check(hdr.names, sizeof(dst->names[0]));
                dbytes_check(hdr.meshes, sizeof(dmeshid_t));
                dbytes_check(hdr.materials, sizeof(dmaterial_t));
                dbytes_check(hdr.lmuvs, sizeof(dlm_uvs_t));
                dbytes_check(hdr.translations, sizeof(dst->translations[0]));
                dbytes_check(hdr.rotations, sizeof(dst->rotations[0]));
                dbytes_check(hdr.scales, sizeof(dst->scales[0]));

                dst->count = len;
                dst->names = perm_calloc(sizeof(dst->names[0]) * len);
                dst->meshes = perm_calloc(sizeof(dst->meshes[0]) * len);
                dst->materials = perm_calloc(sizeof(dst->materials[0]) * len);
                dst->lmUvs = perm_calloc(sizeof(dst->lmUvs[0]) * len);
                dst->matrices = perm_calloc(sizeof(dst->matrices[0]) * len);
                dst->invMatrices = perm_calloc(sizeof(dst->invMatrices[0]) * len);
                dst->translations = perm_calloc(sizeof(dst->translations[0]) * len);
                dst->rotations = perm_calloc(sizeof(dst->rotations[0]) * len);
                dst->scales = perm_calloc(sizeof(dst->scales[0]) * len);

                fstr_seek(fd, hdr.names.offset);
                fstr_read(fd, dst->names, hdr.names.size);
                {
                    fstr_seek(fd, hdr.meshes.offset);
                    dmeshid_t* pim_noalias dmeshids = tmp_realloc(scratch, hdr.names.size);
                    fstr_read(fd, dmeshids, hdr.names.size);
                    for (i32 i = 0; i < hdr.length; ++i)
                    {
                        mesh_load(dmeshids[i].id, dst->meshes + i);
                    }
                    scratch = dmeshids;
                }
                {
                    fstr_seek(fd, hdr.materials.offset);
                    dmaterial_t* pim_noalias dmats = tmp_realloc(scratch, hdr.materials.size);
                    fstr_read(fd, dmats, hdr.materials.size);
                    for (i32 i = 0; i < hdr.length; ++i)
                    {
                        material_t mat = { 0 };
                        const dmaterial_t dmat = dmats[i];
                        mat.st = dmat.st;
                        texture_load(dmat.albedo.id, &mat.albedo);
                        texture_load(dmat.rome.id, &mat.rome);
                        texture_load(dmat.normal.id, &mat.normal);
                        mat.flatAlbedo = dmat.flatAlbedo;
                        mat.flatRome = dmat.flatRome;
                        mat.flags = dmat.flags;
                        mat.ior = dmat.ior;
                        dst->materials[i] = mat;
                    }
                    scratch = dmats;
                }
                {
                    fstr_seek(fd, hdr.lmuvs.offset);
                    dlm_uvs_t* pim_noalias dlmuvs = tmp_realloc(scratch, hdr.lmuvs.size);
                    fstr_read(fd, dlmuvs, hdr.lmuvs.size);
                    for (i32 i = 0; i < hdr.length; ++i)
                    {
                        const dlm_uvs_t dlmuv = dlmuvs[i];
                        lm_uvs_t lmuv = { 0 };
                        if (dlmuv.length < 0)
                        {
                            goto cleanup;
                        }
                        dbytes_check(dlmuv.uvs, sizeof(lmuv.uvs[0]));
                        dbytes_check(dlmuv.indices, sizeof(lmuv.indices[0]));
                        lmuv.length = dlmuv.length;
                        lmuv.uvs = perm_calloc(dlmuv.uvs.size);
                        lmuv.indices = perm_calloc(dlmuv.indices.size);
                        fstr_seek(fd, dlmuv.uvs.offset);
                        fstr_read(fd, lmuv.uvs, dlmuv.uvs.size);
                        fstr_seek(fd, dlmuv.indices.offset);
                        fstr_read(fd, lmuv.indices, dlmuv.indices.size);
                        dst->lmUvs[i] = lmuv;
                    }
                    scratch = dlmuvs;
                }

                fstr_seek(fd, hdr.translations.offset);
                fstr_read(fd, dst->translations, hdr.translations.size);

                fstr_seek(fd, hdr.rotations.offset);
                fstr_read(fd, dst->rotations, hdr.rotations.size);

                fstr_seek(fd, hdr.scales.offset);
                fstr_read(fd, dst->scales, hdr.scales.size);

                loaded = true;
            }
        }
    }
cleanup:
    fstr_close(&fd);
    if (!loaded)
    {
        drawables_del(dst);
    }
    return loaded;
}
