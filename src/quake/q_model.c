#include "quake/q_model.h"
#include "quake/q_packfile.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "common/console.h"
#include "common/stringutil.h"

static const void* OffsetPtr(const void* ptr, i32 bytes)
{
    return (const u8*)ptr + bytes;
}

static mmodel_t* LoadBrushModel(
    const char* name,
    const void* buffer,
    EAlloc allocator);
static void LoadVertices(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadEdges(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadSurfEdges(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadTextures(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadLighting(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadPlanes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadTexInfo(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadMarkSurfaces(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadVisibility(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadLeaves(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadNodes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadClipNodes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadEntities(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void LoadSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void MakeHull0(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model);
static void SetupSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
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
        pim_free(model->submodels);
        pim_free(model->planes);
        pim_free(model->leafs);
        pim_free(model->vertices);
        pim_free(model->edges);
        // todo: recursively free bsp tree

        pim_free(model->texinfo);
        pim_free(model->surfaces);
        pim_free(model->surfedges);
        pim_free(model->clipnodes);

        for (i32 i = 0; i < model->nummarksurfaces; ++i)
        {
            pim_free(model->marksurfaces[i]);
        }
        pim_free(model->marksurfaces);

        for (i32 i = 0; i < MAX_MAP_HULLS; ++i)
        {
            pim_free(model->hulls[i].clipnodes);
            pim_free(model->hulls[i].planes);
        }

        for (i32 i = 0; i < model->numtextures; ++i)
        {
            pim_free(model->textures[i]);
        }
        pim_free(model->textures);

        pim_free(model->visdata);
        pim_free(model->lightdata);
        pim_free(model->entities);
        pim_free(model->cache);

        memset(model, 0, sizeof(*model));
        pim_free(model);
    }
}

static void LoadVertices(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_VERTEXES];
    if (lump.filelen > 0)
    {
        const dvertex_t* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad vertex lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        float4* dst = pim_malloc(allocator, sizeof(*dst) * count);

        for (i32 i = 0; i < count; ++i)
        {
            dst[i].x = src[i].point.x;
            dst[i].y = src[i].point.y;
            dst[i].z = src[i].point.z;
            dst[i].w = 1.0f;
        }

        model->vertices = dst;
        model->numvertices = count;
    }
}

static void LoadEdges(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_EDGES];
    if (lump.filelen > 0)
    {
        const dedge_t* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad edge lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        medge_t* dst = pim_calloc(allocator, sizeof(*dst) * count);
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].v[0] = src[i].v[0];
            dst[i].v[1] = src[i].v[1];
        }

        model->edges = dst;
        model->numedges = count;
    }
}

static void LoadSurfEdges(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_SURFEDGES];
    const i32* src = OffsetPtr(buffer, lump.fileofs);
    if (lump.filelen % sizeof(*src))
    {
        con_logf(LogSev_Error, "mdl", "Bad surfedge lump in %s", model->name);
        return;
    }
    const i32 count = lump.filelen / sizeof(*src);
    ASSERT(count >= 0);

    i32* dst = pim_malloc(allocator, sizeof(*dst) * count);

    for (i32 i = 0; i < count; ++i)
    {
        dst[i] = src[i];
    }

    model->surfedges = dst;
    model->numsurfedges = count;
}

static void LoadTextures(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_TEXTURES];
    if (lump.filelen > 0)
    {
        const dmiptexlump_t* m = OffsetPtr(buffer, lump.fileofs);
        const i32 texCount = m->nummiptex;
        const i32* offsets = m->dataofs;
        ASSERT(texCount >= 0);

        mtexture_t** textures = pim_calloc(allocator, sizeof(*textures) * texCount);
        for (i32 i = 0; i < texCount; ++i)
        {
            const i32 offset = offsets[i];
            if ((offset <= 0) || (offset >= lump.filelen))
            {
                continue;
            }
            const miptex_t* mt = OffsetPtr(m, offset);
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

            mtexture_t* tx = pim_calloc(allocator, sizeof(*tx) + pixels);
            textures[i] = tx;

            ASSERT(sizeof(tx->name) >= sizeof(mt->name));
            memcpy(tx->name, mt->name, sizeof(tx->name));
            tx->width = mt->width;
            tx->height = mt->height;

            const i32 sizeDiff = sizeof(*tx) - sizeof(*mt);
            for (i32 j = 0; j < MIPLEVELS; ++j)
            {
                tx->offsets[j] = mt->offsets[j] + sizeDiff;
            }

            memcpy(tx + 1, mt + 1, pixels);
        }

        model->textures = textures;
        model->numtextures = texCount;

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
                tx2->anim_max = (j+1) * kAnimCycle;
                tx2->anim_next = anims[(j+1)%maxanim];
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
                tx2->anim_max = (j+1) * kAnimCycle;
                tx2->anim_next = altanims[(j+1)%altmax];
                if (maxanim)
                {
                    tx2->alternate_anims = anims[0];
                }
            }
        }
    }
}

static void LoadLighting(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_LIGHTING];
    if (lump.filelen > 0)
    {
        const void* src = OffsetPtr(buffer, lump.fileofs);
        u8* dst = pim_malloc(allocator, lump.filelen);
        memcpy(dst, src, lump.filelen);
        model->lightdata = dst;
        model->lightdatasize = lump.filelen;
    }
}

static void LoadPlanes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_PLANES];
    if (lump.filelen > 0)
    {
        const dplane_t* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad plane lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        float4* dst = pim_malloc(allocator, sizeof(*dst) * count);

        for (i32 i = 0; i < count; ++i)
        {
            dst[i] = f4_v(
                src[i].normal.x,
                src[i].normal.y,
                src[i].normal.z,
                src[i].dist);
        }

        model->planes = dst;
        model->numplanes = count;
    }
}

static void LoadTexInfo(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_TEXINFO];
    if (lump.filelen > 0)
    {
        const dtexinfo_t* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad texinfo lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        mtexinfo_t* dst = pim_calloc(allocator, sizeof(*dst) * count);

        for (i32 i = 0; i < count; ++i)
        {
            dst[i].vecs[0] = f4_load(src[i].vecs[0]);
            dst[i].vecs[1] = f4_load(src[i].vecs[1]);

            float len1 = f4_length3(dst[i].vecs[0]);
            float len2 = f4_length3(dst[i].vecs[1]);
            float len = 0.5f * (len1 + len2);
            float mipadjust = 1.0f;
            if (len < 0.32f)
            {
                mipadjust = 4.0f;
            }
            else if (len < 0.49f)
            {
                mipadjust = 3.0f;
            }
            else if (len < 0.99f)
            {
                mipadjust = 2.0f;
            }
            dst[i].mipadjust = mipadjust;
            dst[i].flags = src[i].flags;

            i32 miptex = src[i].miptex;
            ASSERT(miptex >= 0);
            ASSERT(miptex < model->numtextures);
            if (model->numtextures > 0)
            {
                dst[i].texture = model->textures[miptex];
                if (!dst[i].texture)
                {
                    con_logf(LogSev_Error, "qMdl", "Null texture found in model '%s' at index %d", model->name, miptex);
                }
            }
        }

        model->texinfo = dst;
        model->numtexinfo = count;
    }
}

static void LoadFaces(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_FACES];
    if (lump.filelen > 0)
    {
        const dface_t* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad face lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        msurface_t* dst = pim_calloc(allocator, sizeof(*dst) * count);
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].firstedge = src[i].firstedge;
            dst[i].numedges = src[i].numedges;
            dst[i].flags = 0;

            if (src[i].side)
            {
                dst[i].flags |= SURF_PLANEBACK;
            }

            dst[i].plane = model->planes[src[i].planenum];
            dst[i].texinfo = model->texinfo + src[i].texinfo;

            for (i32 j = 0; j < MAXLIGHTMAPS; ++j)
            {
                dst[i].styles[j] = src[i].styles[j];
            }

            i32 lightofs = src[i].lightofs;
            if (lightofs >= 0)
            {
                dst[i].samples = model->lightdata + lightofs;
            }
        }

        model->surfaces = dst;
        model->numsurfaces = count;
    }
}

static void LoadMarkSurfaces(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_MARKSURFACES];
    if (lump.filelen > 0)
    {
        const u16* src = OffsetPtr(buffer, lump.fileofs);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad MarkSurface lump in %s", model->name);
            return;
        }
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);

        msurface_t** dst = pim_calloc(allocator, sizeof(*dst) * count);
        model->marksurfaces = dst;
        model->nummarksurfaces = count;

        if (count > 32767)
        {
            con_logf(LogSev_Warning, "mdl", "marksurfaces exceeds standard limit of 32767 in %s", model->name);
        }

        const i32 numsurfaces = model->numsurfaces;
        msurface_t* surfaces = model->surfaces;
        for (i32 i = 0; i < count; ++i)
        {
            u16 j = src[i];
            if (j < numsurfaces)
            {
                dst[i] = surfaces + j;
            }
            else
            {
                con_logf(LogSev_Error, "mdl", "More marksurfaces than surfaces in %s", model->name);
                return;
            }
        }
    }
}

static void LoadVisibility(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_VISIBILITY];
    if (lump.filelen > 0)
    {
        const u8* src = OffsetPtr(buffer, lump.fileofs);
        u8* dst = pim_malloc(allocator, lump.filelen);
        memcpy(dst, src, lump.filelen);
        model->visdata = dst;
        model->visdatasize = lump.filelen;
    }
}

static void LoadLeaves(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    //lump_t lump = header->lumps[LUMP_LEAFS];
    //if (lump.filelen > 0)
    //{
    //    const dleaf_t* src = OffsetPtr(buffer, lump.fileofs);
    //    if (lump.filelen % sizeof(*src))
    //    {
    //        con_logf(LogSev_Error, "mdl", "Bad leaves lump in %s", model->name);
    //        return;
    //    }
    //    const i32 count = lump.filelen / sizeof(*src);
    //    ASSERT(count >= 0);
    //    if (count > 32767)
    //    {
    //        con_logf(LogSev_Error, "mdl", "")
    //    }

    //    mleaf_t* dst = pim_calloc(allocator, sizeof(*dst) * count);
    //}
}

static void LoadNodes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static void LoadClipNodes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static void LoadEntities(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static void LoadSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static void MakeHull0(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static void SetupSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{

}

static mmodel_t* LoadBrushModel(
    const char* name,
    const void* buffer,
    EAlloc allocator)
{
    const dheader_t* header = buffer;
    ASSERT(header->version == BSPVERSION);

    mmodel_t* model = pim_calloc(allocator, sizeof(*model));
    StrCpy(ARGS(model->name), name);

    LoadVertices(buffer, allocator, header, model);
    LoadEdges(buffer, allocator, header, model);
    LoadSurfEdges(buffer, allocator, header, model);
    LoadTextures(buffer, allocator, header, model);
    LoadLighting(buffer, allocator, header, model);
    LoadPlanes(buffer, allocator, header, model);
    LoadTexInfo(buffer, allocator, header, model);
    LoadFaces(buffer, allocator, header, model);
    LoadMarkSurfaces(buffer, allocator, header, model);
    LoadVisibility(buffer, allocator, header, model);
    LoadLeaves(buffer, allocator, header, model);
    LoadNodes(buffer, allocator, header, model);
    LoadClipNodes(buffer, allocator, header, model);
    LoadEntities(buffer, allocator, header, model);
    LoadSubModels(buffer, allocator, header, model);
    MakeHull0(buffer, allocator, header, model);
    SetupSubModels(buffer, allocator, header, model);

    return model;
}
