#include "quake/q_model.h"
#include "quake/q_packfile.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "common/console.h"
#include "common/stringutil.h"

#include <string.h>

static const void* OffsetPtr(const void* ptr, i32 bytes)
{
    ASSERT(bytes >= 0);
    return (const u8*)ptr + bytes;
}

static bool CheckLump(
    const char* modelname,
    const char* lumpname,
    lump_t lump,
    i32 stride,
    i32 limit,
    i32* countOut)
{
    *countOut = 0;
    i32 count = lump.filelen / stride;
    if ((count < 0) || (lump.filelen % stride))
    {
        con_logf(LogSev_Error, "mdl", "Bad '%s' lump in model '%s'", lumpname, modelname);
        return false;
    }
    if (count > limit)
    {
        con_logf(LogSev_Error, "mdl", "Lump '%s' count %d exceeds limit of %d in model '%s'", lumpname, count, limit, modelname);
        return false;
    }
    *countOut = count;
    return count > 0;
}

static mmodel_t* LoadBrushModel(
    const char* name,
    const void* buffer,
    EAlloc allocator);
static bool LoadVertices(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadFaces(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadEdges(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadSurfEdges(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadTextures(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadTexInfo(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);
static bool LoadEntities(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model);

mmodel_t* LoadModel(
    const char* name,
    const void* buffer,
    EAlloc allocator)
{
    ASSERT(name);
    ASSERT(buffer);
    const i32 id = *(const i32*)buffer;
    switch (id)
    {
    default:
        ASSERT(false);
        return NULL;
    case IDPOLYHEADER:
        // alias model
        return NULL;
    case IDSPRITEHEADER:
        // sprite model
        return NULL;
    case BSPVERSION:
        // brush model
        return LoadBrushModel(name, buffer, allocator);
    }
}

void FreeModel(mmodel_t* model)
{
    if (model)
    {
        pim_free(model->vertices);
        pim_free(model->edges);
        pim_free(model->texinfo);
        pim_free(model->surfaces);
        pim_free(model->surfedges);
        for (i32 i = 0; i < model->numtextures; ++i)
        {
            pim_free(model->textures[i]);
        }
        pim_free(model->textures);
        pim_free(model->entities);

        memset(model, 0, sizeof(*model));
        pim_free(model);
    }
}

static mmodel_t* LoadBrushModel(
    const char* name,
    const void* buffer,
    EAlloc allocator)
{
    dheader_t const *const header = buffer;
    if (header->version != BSPVERSION)
    {
        ASSERT(false);
        return NULL;
    }

    mmodel_t* model = pim_calloc(allocator, sizeof(*model));
    StrCpy(ARGS(model->name), name);

    LoadVertices(buffer, allocator, header, model);
    LoadEdges(buffer, allocator, header, model);
    LoadSurfEdges(buffer, allocator, header, model);
    LoadTextures(buffer, allocator, header, model);
    LoadTexInfo(buffer, allocator, header, model);
    LoadFaces(buffer, allocator, header, model);
    LoadEntities(buffer, allocator, header, model);

    return model;
}

static bool LoadVertices(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_VERTEXES];
    dvertex_t const *const pim_noalias src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "vertices", lump, sizeof(src[0]), MAX_MAP_VERTS, &count))
    {
        float4 *const pim_noalias dst = pim_malloc(allocator, sizeof(dst[0]) * count);
        model->vertices = dst;
        model->numvertices = count;
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].x = src[i].point[0];
            dst[i].y = src[i].point[1];
            dst[i].z = src[i].point[2];
            dst[i].w = 1.0f;
        }
        return true;
    }
    return false;
}

static bool LoadEdges(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_EDGES];
    dedge_t const *const pim_noalias src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "edges", lump, sizeof(src[0]), MAX_MAP_EDGES, &count))
    {
        medge_t *const pim_noalias dst = pim_calloc(allocator, sizeof(dst[0]) * count);
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].v[0] = src[i].v[0];
            dst[i].v[1] = src[i].v[1];
        }
        model->edges = dst;
        model->numedges = count;
        return true;
    }
    return false;
}

static bool LoadSurfEdges(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_SURFEDGES];
    i32 const *const pim_noalias src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "surfedges", lump, sizeof(src[0]), MAX_MAP_SURFEDGES, &count))
    {
        i32 *const pim_noalias dst = pim_malloc(allocator, sizeof(dst[0]) * count);
        memcpy(dst, src, sizeof(dst[0]) * count);
        model->surfedges = dst;
        model->numsurfedges = count;
        return true;
    }
    return false;
}

static bool LoadTextures(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_TEXTURES];
    if (lump.filelen > 0)
    {
        dmiptexlump_t const *const m = OffsetPtr(buffer, lump.fileofs);
        const i32 texCount = m->nummiptex;
        i32 const *const pim_noalias offsets = m->dataofs;
        ASSERT(texCount >= 0);
        if (texCount > MAX_MAP_TEXTURES)
        {
            con_logf(LogSev_Error, "mdl", "texture count %d exceeds limit of %d in %s", texCount, MAX_MAP_TEXTURES, model->name);
            return false;
        }

        mtexture_t **const pim_noalias textures = pim_calloc(allocator, sizeof(textures[0]) * texCount);
        model->textures = textures;
        model->numtextures = texCount;

        for (i32 i = 0; i < texCount; ++i)
        {
            const i32 offset = offsets[i];
            if ((offset < sizeof(*m)) || (offset >= lump.filelen))
            {
                continue;
            }
            miptex_t const *const pim_noalias mt = OffsetPtr(m, offset);
            if ((mt->width & 15) || (mt->height & 15))
            {
                con_logf(LogSev_Error, "mdl", "Texture size is not multiple of 16: %s", mt->name);
                continue;
            }

            // equivalent to (x*x) + (x/2 * x/2) + (x/4 * x/4) + (x/8 * x/8)
            // each texel is a single byte. decodable to a color by using a palette.
            const i32 pixels = (mt->width * mt->height / 64) * 85;
            ASSERT(pixels > 0);
            if ((pixels + offset + sizeof(*mt)) > lump.filelen)
            {
                con_logf(LogSev_Error, "mdl", "Texture extends past end of lump: %s", mt->name);
                continue;
            }

            mtexture_t *const pim_noalias tx = pim_calloc(allocator, sizeof(*tx) + pixels);
            textures[i] = tx;

            SASSERT(sizeof(tx->name) >= sizeof(mt->name));
            memcpy(tx->name, mt->name, sizeof(tx->name));
            tx->width = mt->width;
            tx->height = mt->height;

            memcpy(tx + 1, mt + 1, pixels);
        }

        // link together the texture animations
        mtexture_t* anims[10];
        mtexture_t* altanims[10];
        for (i32 i = 0; i < texCount; ++i)
        {
            // animated textures have names in the format of "+0lava", "+1lava", etc
            // alt animated textures have names in the format of "+awater", "+bwater", etc
            // animated textures have at most 10 keyframes (0-9 or a-j)
            // warped/turbulent textures have names in the format of "*slip" and are not keyframed
            mtexture_t* tx = textures[i];
            if (!tx || tx->name[0] != '+')
            {
                continue;
            }
            if (tx->anim_next)
            {
                continue;
            }

            memset(anims, 0, sizeof(anims));
            memset(altanims, 0, sizeof(altanims));

            char maxanim = ChrUp(tx->name[1]);
            char altmax = 0;
            if (IsDigit(maxanim))
            {
                maxanim -= '0';
                altmax = 0;
                anims[maxanim] = tx;
                maxanim++;
            }
            else if (maxanim >= 'A' && maxanim <= 'J')
            {
                altmax = maxanim - 'A';
                maxanim = 0;
                altanims[altmax] = tx;
                altmax++;
            }
            else
            {
                con_logf(LogSev_Error, "mdl", "Bad animating texture %s", tx->name);
                continue;
            }

            for (i32 j = i + 1; j < texCount; ++j)
            {
                mtexture_t* tx2 = textures[j];
                if (!tx2 || tx2->name[0] != '+')
                {
                    continue;
                }
                if (StrCmp(tx->name + 2, sizeof(tx->name) - 2, tx2->name + 2))
                {
                    continue;
                }

                char num = ChrUp(tx2->name[1]);
                if (IsDigit(num))
                {
                    num -= '0';
                    anims[num] = tx2;
                    if ((num + 1) > maxanim)
                    {
                        maxanim = num + 1;
                    }
                }
                else if (num >= 'A' && num <= 'J')
                {
                    num -= 'A';
                    altanims[num] = tx2;
                    if ((num + 1) > altmax)
                    {
                        altmax = num + 1;
                    }
                }
                else
                {
                    con_logf(LogSev_Error, "mdl", "Bad animating texture: %s", tx2->name);
                    continue;
                }
            }

            const i32 kAnimCycle = 2;
            for (i32 j = 0; j < maxanim; ++j)
            {
                mtexture_t* tx2 = anims[j];
                if (!tx2)
                {
                    con_logf(LogSev_Error, "mdl", "Missing animtex frame %d of %s", j, tx->name);
                    continue;
                }
                tx2->anim_total = maxanim * kAnimCycle;
                tx2->anim_min = j * kAnimCycle;
                tx2->anim_max = (j + 1) * kAnimCycle;
                tx2->anim_next = anims[(j + 1) % maxanim];
                if (altmax)
                {
                    tx2->alternate_anims = altanims[0];
                }
            }
            for (i32 j = 0; j < altmax; ++j)
            {
                mtexture_t* tx2 = altanims[j];
                if (!tx2)
                {
                    con_logf(LogSev_Error, "mdl", "Missing alt animtex frame %d of %s", j, tx->name);
                    continue;
                }
                tx2->anim_total = altmax * kAnimCycle;
                tx2->anim_min = j * kAnimCycle;
                tx2->anim_max = (j + 1) * kAnimCycle;
                tx2->anim_next = altanims[(j + 1) % altmax];
                if (maxanim)
                {
                    tx2->alternate_anims = anims[0];
                }
            }
        }

        return true;
    }

    return false;
}

static bool LoadTexInfo(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_TEXINFO];
    dtexinfo_t const *const pim_noalias src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "texinfo", lump, sizeof(src[0]), MAX_MAP_TEXINFO, &count))
    {
        mtexinfo_t *const pim_noalias dst = pim_calloc(allocator, sizeof(dst[0]) * count);
        model->texinfo = dst;
        model->numtexinfo = count;

        const i32 numtextures = model->numtextures;
        mtexture_t const *const *const textures = model->textures;
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].vecs[0] = f4_load(src[i].vecs[0]);
            dst[i].vecs[1] = f4_load(src[i].vecs[1]);
            if (numtextures > 0)
            {
                i32 miptex = i1_clamp(src[i].miptex, 0, numtextures - 1);
                dst[i].texture = textures[miptex];
                if (!dst[i].texture)
                {
                    con_logf(LogSev_Error, "mdl", "Null texture found in model '%s' at index %d", model->name, miptex);
                }
            }
        }
        return true;
    }
    return false;
}

static bool LoadFaces(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_FACES];
    dface_t const *const src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "faces", lump, sizeof(src[0]), MAX_MAP_FACES, &count))
    {
        msurface_t *const pim_noalias dst = pim_calloc(allocator, sizeof(dst[0]) * count);
        model->surfaces = dst;
        model->numsurfaces = count;

        const i32 numtexinfo = model->numtexinfo;
        mtexinfo_t const *const texinfos = model->texinfo;
        const i32 numsurfedges = model->numsurfedges;

        for (i32 i = 0; i < count; ++i)
        {
            i32 first = i1_clamp(src[i].firstedge, 0, numsurfedges - 1);
            i32 num = i1_clamp(src[i].numedges, 0, numsurfedges - first);
            dst[i].firstedge = first;
            dst[i].numedges = num;

            i32 iTexInfo = src[i].texinfo;
            if ((iTexInfo >= 0) && (iTexInfo < numtexinfo))
            {
                dst[i].texinfo = texinfos + iTexInfo;
            }
        }
        return true;
    }
    return false;
}

static bool LoadEntities(
    const void* buffer,
    EAlloc allocator,
    dheader_t const *const header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_ENTITIES];
    char const *const pim_noalias src = OffsetPtr(buffer, lump.fileofs);
    i32 count = 0;
    if (CheckLump(model->name, "entities", lump, sizeof(src[0]), MAX_MAP_ENTSTRING, &count))
    {
        char *const pim_noalias dst = pim_malloc(allocator, count + 1);
        memcpy(dst, src, count);
        dst[count] = 0;
        model->entities = dst;
        model->entitiessize = count;
        return true;
    }
    return false;
}

