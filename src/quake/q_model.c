#include "quake/q_model.h"
#include "quake/q_packfile.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "common/console.h"
#include "common/stringutil.h"

#include <string.h>

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
static void LoadFaces(
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
static void LoadTree(
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
        pim_free(model->planes);
        pim_free(model->vertices);
        pim_free(model->edges);
        pim_free(model->nodes);

        pim_free(model->texinfo);
        pim_free(model->surfaces);
        pim_free(model->surfedges);
        pim_free(model->clipnodes);
        pim_free(model->marksurfaces);

        // hull 0 has a dedicated clipnode array
        pim_free(model->hulls[0].clipnodes);

        for (i32 i = 0; i < model->numtextures; ++i)
        {
            pim_free(model->textures[i]);
        }
        pim_free(model->textures);

        pim_free(model->visdata);
        pim_free(model->lightdata);
        pim_free(model->entities);
        pim_free(model->cache);
        pim_free(model->dsubmodels);
        pim_free(model->msubmodels);

        memset(model, 0, sizeof(*model));
        pim_free(model);
    }
}

static mmodel_t* LoadBrushModel(
    const char* name,
    const void* buffer,
    EAlloc allocator)
{
    const dheader_t* header = buffer;
    if (header->version != BSPVERSION)
    {
        ASSERT(false);
        return NULL;
    }

    mmodel_t* model = pim_calloc(allocator, sizeof(*model));
    StrCpy(ARGS(model->name), name);

    model->type = mod_brush;
    model->numframes = 2;

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
    LoadTree(buffer, allocator, header, model);
    LoadClipNodes(buffer, allocator, header, model);
    LoadEntities(buffer, allocator, header, model);
    LoadSubModels(buffer, allocator, header, model);
    MakeHull0(buffer, allocator, header, model);
    SetupSubModels(buffer, allocator, header, model);

    return model;
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad vertex lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_VERTS)
        {
            con_logf(LogSev_Error, "mdl", "vertex count %d exceeds limit of %d in %s", count, MAX_MAP_VERTS, model->name);
            return;
        }

        float4* pim_noalias dst = pim_malloc(allocator, sizeof(*dst) * count);
        for (i32 i = 0; i < count; ++i)
        {
            dst[i].x = src[i].point[0];
            dst[i].y = src[i].point[1];
            dst[i].z = src[i].point[2];
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad edge lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_EDGES)
        {
            con_logf(LogSev_Error, "mdl", "edges count %d exceeds limit of %d in %s", count, MAX_MAP_EDGES, model->name);
            return;
        }

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
    const i32 count = lump.filelen / sizeof(*src);
    ASSERT(count >= 0);
    if (lump.filelen % sizeof(*src))
    {
        con_logf(LogSev_Error, "mdl", "Bad surfedge lump in %s", model->name);
        return;
    }
    if (count > MAX_MAP_SURFEDGES)
    {
        con_logf(LogSev_Error, "mdl", "surfedges count %d exceeds limit of %d in %s", count, MAX_MAP_SURFEDGES, model->name);
        return;
    }

    i32* dst = pim_malloc(allocator, sizeof(*dst) * count);
    memcpy(dst, src, sizeof(*dst) * count);
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
        if (texCount > MAX_MAP_TEXTURES)
        {
            con_logf(LogSev_Error, "mdl", "texture count %d exceeds limit of %d in %s", texCount, MAX_MAP_TEXTURES, model->name);
            return;
        }

        mtexture_t** textures = pim_calloc(allocator, sizeof(*textures) * texCount);
        model->textures = textures;
        model->numtextures = texCount;

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
        if (lump.filelen > MAX_MAP_LIGHTING)
        {
            con_logf(LogSev_Error, "mdl", "lighting size %d exceeds limit of %d in %s", lump.filelen, MAX_MAP_LIGHTING, model->name);
            return;
        }
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad plane lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_PLANES)
        {
            con_logf(LogSev_Error, "mdl", "planes count %d exceeds limit of %d in %s", count, MAX_MAP_PLANES, model->name);
            return;
        }

        float4* pim_noalias dst = pim_malloc(allocator, sizeof(*dst) * count);
        for (i32 i = 0; i < count; ++i)
        {
            dst[i] = f4_v(
                src[i].normal[0],
                src[i].normal[1],
                src[i].normal[2],
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad texinfo lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_TEXINFO)
        {
            con_logf(LogSev_Error, "mdl", "texinfo count %d exceeds limit of %d in %s", count, MAX_MAP_TEXINFO, model->name);
            return;
        }

        mtexinfo_t* dst = pim_calloc(allocator, sizeof(*dst) * count);
        model->texinfo = dst;
        model->numtexinfo = count;

        const i32 numtextures = model->numtextures;
        mtexture_t** textures = model->textures;

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

            if (model->numtextures > 0)
            {
                i32 miptex = i1_clamp(src[i].miptex, 0, numtextures - 1);
                dst[i].texture = textures[miptex];
                if (!dst[i].texture)
                {
                    con_logf(LogSev_Error, "mdl", "Null texture found in model '%s' at index %d", model->name, miptex);
                }
            }
        }
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad face lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_FACES)
        {
            con_logf(LogSev_Error, "mdl", "faces count %d exceeds limit of %d in %s", count, MAX_MAP_FACES, model->name);
            return;
        }

        msurface_t* dst = pim_calloc(allocator, sizeof(*dst) * count);
        model->surfaces = dst;
        model->numsurfaces = count;

        const i32 numtexinfo = model->numtexinfo;
        mtexinfo_t* texinfos = model->texinfo;
        const i32 numsurfedges = model->numsurfedges;
        const i32 numplanes = model->numplanes;
        const i32 lightdatasize = model->lightdatasize;

        for (i32 i = 0; i < count; ++i)
        {
            dst[i].side = src[i].side;
            dst[i].planenum = i1_clamp(src[i].planenum, 0, numplanes - 1);
            i32 firstedge = i1_clamp(src[i].firstedge, 0, numsurfedges - 1);
            dst[i].firstedge = firstedge;
            dst[i].numedges = i1_clamp(src[i].numedges, 0, numsurfedges - firstedge);

            i32 texinfo = src[i].texinfo;
            if ((texinfo >= 0) && (texinfo < numtexinfo))
            {
                dst[i].texinfo = texinfos + texinfo;
            }

            for (i32 j = 0; j < MAXLIGHTMAPS; ++j)
            {
                dst[i].styles[j] = src[i].styles[j];
            }

            i32 lightofs = src[i].lightofs;
            if ((lightofs >= 0) && (lightofs < lightdatasize))
            {
                dst[i].samples = model->lightdata + lightofs;
            }

            dst[i].flags = 0;
            if (src[i].side)
            {
                dst[i].flags |= SURF_PLANEBACK;
            }
        }
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
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad MarkSurface lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_MARKSURFACES)
        {
            con_logf(LogSev_Error, "mdl", "marksurfaces count %d exceeds limit of %d in %s", count, MAX_MAP_MARKSURFACES, model->name);
            return;
        }

        msurface_t** dst = pim_calloc(allocator, sizeof(*dst) * count);
        model->marksurfaces = dst;
        model->nummarksurfaces = count;

        const i32 numsurfaces = model->numsurfaces;
        msurface_t* surfaces = model->surfaces;
        if (numsurfaces > 0)
        {
            for (i32 i = 0; i < count; ++i)
            {
                i32 j = i1_clamp(src[i], 0, numsurfaces - 1);
                dst[i] = surfaces + j;
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
        if (lump.filelen > MAX_MAP_VISIBILITY)
        {
            con_logf(LogSev_Error, "mdl", "Visibility size %d exceeds limit of %d in %s", lump.filelen, MAX_MAP_VISIBILITY, model->name);
            return;
        }
        const u8* src = OffsetPtr(buffer, lump.fileofs);
        u8* dst = pim_malloc(allocator, lump.filelen);
        memcpy(dst, src, lump.filelen);
        model->visdata = dst;
        model->visdatasize = lump.filelen;
    }
}

static void LoadTree(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t leafLump = header->lumps[LUMP_LEAFS];
    lump_t nodeLump = header->lumps[LUMP_NODES];
    if ((leafLump.filelen > 0) && (nodeLump.filelen > 0))
    {
        const dleaf_t* srcLeafs = OffsetPtr(buffer, leafLump.fileofs);
        const dnode_t* srcNodes = OffsetPtr(buffer, nodeLump.fileofs);
        const i32 numleafs = leafLump.filelen / sizeof(*srcLeafs);
        const i32 numnodes = nodeLump.filelen / sizeof(*srcNodes);
        if (leafLump.filelen % sizeof(*srcLeafs))
        {
            con_logf(LogSev_Error, "mdl", "Bad leaves lump in %s", model->name);
            return;
        }
        if (nodeLump.filelen % sizeof(*srcNodes))
        {
            con_logf(LogSev_Error, "mdl", "Bad nodes lump in %s", model->name);
            return;
        }
        if (numleafs > MAX_MAP_LEAFS)
        {
            con_logf(LogSev_Error, "mdl", "Leaf node count %d exceeds limit of %d in %s", numleafs, MAX_MAP_LEAFS, model->name);
            return;
        }
        if (numnodes > MAX_MAP_NODES)
        {
            con_logf(LogSev_Error, "mdl", "node count %d exceeds limit of %d in %s", numnodes, MAX_MAP_NODES, model->name);
            return;
        }

        const i32 total = numleafs + numnodes;

        mnode_t* nodes = model->nodes;
        nodes = pim_calloc(allocator, sizeof(*nodes) * total);
        model->nodes = nodes;
        model->numleafs = numleafs;
        model->numnodes = numnodes;

        const i32 nummarksurfaces = model->nummarksurfaces;
        const i32 numsurfaces = model->numsurfaces;
        msurface_t** marksurfaces = model->marksurfaces;

        mnode_t* dstLeafs = nodes + numnodes;
        for (i32 i = 0; i < numleafs; ++i)
        {
            const dleaf_t srcLeaf = srcLeafs[i];
            mnode_t dstLeaf = { 0 };

            dstLeaf.c.contents = srcLeaf.contents;
            // contents < 0 => leaf
            if (srcLeaf.contents >= 0)
            {
                dstLeaf.c.contents = CONTENTS_EMPTY;
            }

            for (i32 j = 0; j < 3; ++j)
            {
                dstLeaf.c.mins[j] = srcLeaf.mins[j];
                dstLeaf.c.maxs[j] = srcLeaf.maxs[j];
            }

            dstLeaf.u.leaf.visofs = srcLeaf.visofs;
            dstLeaf.u.leaf.firstmarksurface = srcLeaf.firstmarksurface;
            dstLeaf.u.leaf.nummarksurfaces = srcLeaf.nummarksurfaces;
            for (i32 j = 0; j < NUM_AMBIENTS; ++j)
            {
                dstLeaf.u.leaf.ambient_level[j] = srcLeaf.ambient_level[j];
            }

            if (nummarksurfaces > 0)
            {
                if ((dstLeaf.c.contents == CONTENTS_WATER) ||
                    (dstLeaf.c.contents == CONTENTS_SLIME))
                {
                    i32 first = i1_clamp(srcLeaf.firstmarksurface, 0, nummarksurfaces - 1);
                    i32 num = i1_clamp(srcLeaf.nummarksurfaces, 0, nummarksurfaces - first);
                    for (i32 j = 0; j < num; ++j)
                    {
                        msurface_t* mark = marksurfaces[first + j];
                        if (mark)
                        {
                            mark->flags |= SURF_UNDERWATER;
                        }
                    }
                }
            }

            dstLeafs[i] = dstLeaf;
        }

        mnode_t* dstNodes = nodes + 0;
        for (i32 i = 0; i < numnodes; ++i)
        {
            const dnode_t srcNode = srcNodes[i];
            mnode_t dstNode = { 0 };

            // >= 0 contents => node
            dstNode.c.contents = 0;
            for (i32 j = 0; j < 3; ++j)
            {
                dstNode.c.mins[j] = srcNode.mins[j];
                dstNode.c.maxs[j] = srcNode.maxs[j];
            }
            dstNode.u.node.planenum = srcNode.planenum;
            dstNode.u.node.firstface = srcNode.firstface;
            dstNode.u.node.numfaces = srcNode.numfaces;

            for (i32 j = 0; j < 2; ++j)
            {
                i32 p = srcNode.children[j];
                if (p < 0)
                {
                    // leaf
                    p = 0xffff - p;
                    if (p < numleafs)
                    {
                        i32 index = numnodes + p;
                        dstNode.u.node.children[j] = dstLeafs + p;
                        nodes[index].c.parent = dstNodes + i;
                    }
                }
                else
                {
                    i32 index = p;
                    if (index < numnodes)
                    {
                        dstNode.u.node.children[j] = dstNodes + index;
                        nodes[index].c.parent = dstNodes + i;
                    }
                }
            }

            dstNodes[i] = dstNode;
        }
    }
}

static void LoadClipNodes(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_CLIPNODES];
    if (lump.filelen > 0)
    {
        const dclipnode_t* src = OffsetPtr(buffer, lump.fileofs);
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad clipnode lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_CLIPNODES)
        {
            con_logf(LogSev_Error, "mdl", "clipnode count %d exceeds limit of %d in %s", count, MAX_MAP_CLIPNODES, model->name);
            return;
        }

        if (count > 0)
        {
            dclipnode_t* dst = pim_malloc(allocator, sizeof(*dst) * count);
            memcpy(dst, src, sizeof(src[0]) * count);

            model->clipnodes = dst;
            model->numclipnodes = count;

            mhull_t* hull = model->hulls + 1;
            hull->clipnodes = dst;
            hull->firstclipnode = 0;
            hull->lastclipnode = count - 1;
            hull->planes = model->planes;
            hull->clip_mins.x = -16.0f;
            hull->clip_mins.y = -16.0f;
            hull->clip_mins.z = -24.0f;
            hull->clip_maxs.x = 16.0f;
            hull->clip_maxs.y = 16.0f;
            hull->clip_maxs.z = 32.0f;

            hull = model->hulls + 2;
            hull->clipnodes = dst;
            hull->firstclipnode = 0;
            hull->lastclipnode = count - 1;
            hull->planes = model->planes;
            hull->clip_mins.x = -32.0f;
            hull->clip_mins.y = -32.0f;
            hull->clip_mins.z = -24.0f;
            hull->clip_maxs.x = 32.0f;
            hull->clip_maxs.y = 32.0f;
            hull->clip_maxs.z = 64.0f;
        }
    }
}

static void LoadEntities(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_ENTITIES];
    if (lump.filelen > 0)
    {
        if (lump.filelen > MAX_MAP_ENTSTRING)
        {
            con_logf(LogSev_Error, "mdl", "entity string size %d exceeds limit of %d in %s", lump.filelen, MAX_MAP_ENTSTRING, model->name);
            return;
        }
        const char* src = OffsetPtr(buffer, lump.fileofs);
        char* dst = pim_malloc(allocator, lump.filelen + 1);
        memcpy(dst, src, lump.filelen);
        dst[lump.filelen] = 0;
        model->entities = dst;
        model->entitiessize = lump.filelen;
    }
}

static void LoadSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    lump_t lump = header->lumps[LUMP_MODELS];
    if (lump.filelen > 0)
    {
        const dmodel_t* src = OffsetPtr(buffer, lump.fileofs);
        const i32 count = lump.filelen / sizeof(*src);
        ASSERT(count >= 0);
        if (lump.filelen % sizeof(*src))
        {
            con_logf(LogSev_Error, "mdl", "Bad submodel lump in %s", model->name);
            return;
        }
        if (count > MAX_MAP_MODELS)
        {
            con_logf(LogSev_Error, "mdl", "submodel count %d exceeds limit of %d in %s", count, MAX_MAP_MODELS, model->name);
            return;
        }

        dmodel_t* dst = pim_malloc(allocator, sizeof(*dst) * count);
        memcpy(dst, src, sizeof(*dst) * count);
        model->dsubmodels = dst;
        model->numsubmodels = count;
    }
}

static void MakeHull0(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    mhull_t* hull = model->hulls + 0;
    const mnode_t* src = model->nodes;
    const i32 numnodes = model->numnodes;
    if (numnodes > 0)
    {
        dclipnode_t* dst = pim_calloc(allocator, sizeof(*dst) * numnodes);

        hull->clipnodes = dst;
        hull->firstclipnode = 0;
        hull->lastclipnode = numnodes - 1;
        hull->planes = model->planes;

        for (i32 i = 0; i < numnodes; ++i)
        {
            dst[i].planenum = src[i].u.node.planenum;
            for (i32 j = 0; j < 2; ++j)
            {
                const mnode_t* child = src[i].u.node.children[j];
                if (child)
                {
                    if (child->c.contents < 0)
                    {
                        dst[i].children[j] = child->c.contents;
                    }
                    else
                    {
                        i32 index = (i32)(child - src);
                        dst[i].children[j] = index;
                    }
                }
            }
        }
    }
}

static float RadiusFromBounds(const float* mins, const float* maxs)
{
    float4 corner =
    {
        f1_max(f1_abs(mins[0]), f1_abs(maxs[0])),
        f1_max(f1_abs(mins[1]), f1_abs(maxs[1])),
        f1_max(f1_abs(mins[1]), f1_abs(maxs[2])),
    };
    return f4_length3(corner);
}

static void SetupSubModels(
    const void* buffer,
    EAlloc allocator,
    const dheader_t* header,
    mmodel_t* model)
{
    // the submodels are slightly mutated copies of the main model
    // which point into sections of the main data

    const i32 numsubmodels = model->numsubmodels;
    if (numsubmodels > 0)
    {
        model->msubmodels = pim_malloc(
            allocator,
            sizeof(model->msubmodels[0]) * numsubmodels);

        const dmodel_t* dsubmodels = model->dsubmodels;
        mmodel_t* msubmodels = model->msubmodels;

        for (i32 i = 0; i < numsubmodels; ++i)
        {
            mmodel_t* mod = msubmodels + i;
            const dmodel_t* bm = dsubmodels + i;
            *mod = *model;
            SPrintf(ARGS(mod->name), "*%d", i);

            mod->hulls[0].firstclipnode = bm->headnode[0];
            for (i32 j = 1; j < MAX_MAP_HULLS; ++j)
            {
                mod->hulls[j].firstclipnode = bm->headnode[j];
                mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
            }

            mod->firstmodelsurface = bm->firstface;
            mod->nummodelsurfaces = bm->numfaces;

            for (i32 j = 0; j < 3; ++j)
            {
                mod->maxs[j] = bm->maxs[j];
                mod->mins[j] = bm->mins[j];
            }
            mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
            mod->numleafs = bm->visleafs;
        }
    }
}
