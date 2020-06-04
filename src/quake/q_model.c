#include "quake/q_model.h"
#include "quake/q_packfile.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "common/console.h"
#include "common/stringutil.h"

static mmodel_t* LoadBrushModel(const char* name, const void* buffer, EAlloc allocator);

mmodel_t* LoadModel(const char* name, const void* buffer, EAlloc allocator)
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

static const void* OffsetPtr(const void* ptr, i32 bytes)
{
    return (const u8*)ptr + bytes;
}

static mmodel_t* LoadBrushModel(const char* name, const void* buffer, EAlloc allocator)
{
    const dheader_t* header = buffer;
    ASSERT(header->version == BSPVERSION);

    mmodel_t* model = pim_calloc(allocator, sizeof(*model));
    StrCpy(ARGS(model->name), name);

    // load vertices
    {
        lump_t lump = header->lumps[LUMP_VERTEXES];
        const dvertex_t* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
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

    // load edges
    {
        lump_t lump = header->lumps[LUMP_EDGES];
        const dedge_t* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
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

    // load surface edges
    {
        lump_t lump = header->lumps[LUMP_SURFEDGES];
        const i32* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
        ASSERT(count >= 0);

        i32* dst = pim_malloc(allocator, sizeof(*dst) * count);

        for (i32 i = 0; i < count; ++i)
        {
            dst[i] = src[i];
        }

        model->surfedges = dst;
        model->numsurfedges = count;
    }

    // load textures
    {
        lump_t lump = header->lumps[LUMP_TEXTURES];
        if (lump.filelen > 0)
        {
            const dmiptexlump_t* m = OffsetPtr(buffer, lump.fileofs);
            const i32 texCount = m->nummiptex;
            ASSERT(texCount >= 0);

            mtexture_t** textures = pim_calloc(
                allocator, sizeof(*textures) * texCount);
            for (i32 i = 0; i < texCount; ++i)
            {
                const i32 offset = m->dataofs[i];
                if (offset <= 0)
                {
                    continue;
                }
                const miptex_t* mt = OffsetPtr(m, offset);

                ASSERT((mt->width & 15) == 0);
                ASSERT((mt->height & 15) == 0);
                // equivalent to (x*x) + (x/2 * x/2) + (x/4 * x/4) + (x/8 * x/8)
                // each texel is a single byte. decodable to a color by using a palette.
                const i32 pixels = (mt->width * mt->height / 64) * 85;
                ASSERT(pixels > 0);

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
        }
    }

    // load lighting
    {
        lump_t lump = header->lumps[LUMP_LIGHTING];
        if (lump.filelen > 0)
        {
            u8* dst = pim_malloc(allocator, lump.filelen);
            const void* src = OffsetPtr(buffer, lump.fileofs);
            memcpy(dst, src, lump.filelen);

            model->lightdata = dst;
        }
    }

    // load planes
    {
        lump_t lump = header->lumps[LUMP_PLANES];
        const dplane_t* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
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

    // load texinfo
    {
        lump_t lump = header->lumps[LUMP_TEXINFO];
        const dtexinfo_t* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
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

    // load faces
    {
        lump_t lump = header->lumps[LUMP_FACES];
        const dface_t* src = OffsetPtr(buffer, lump.fileofs);

        const i32 count = lump.filelen / sizeof(*src);
        ASSERT((lump.filelen % sizeof(*src)) == 0);
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

    // load mark surfaces
    {

    }

    // load visibility
    {

    }

    // load leaves
    {

    }

    // load nodes
    {

    }

    // load clipnodes
    {

    }

    // load entities
    {

    }

    // load submodels
    {

    }

    // make hull0
    {

    }

    // setup submodels
    {

    }

    return model;
}
