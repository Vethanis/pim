#include "rendering/model.h"
#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/quat_funcs.h"
#include "math/color.h"
#include "math/sdf.h"
#include "math/sampling.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/drawable.h"
#include "allocator/allocator.h"
#include "common/iid.h"
#include "assets/asset_system.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "common/sort.h"

#include "quake/q_model.h"

#include <string.h>

typedef struct mat_preset_s
{
    const char* name;
    float roughness;
    float occlusion;
    float metallic;
    float emission;
} mat_preset_t;

#define kMatGen     0.5f, 1.0f, 0.0f, 0.0f,
#define kMatRough   0.75f, 1.0f, 0.0f, 0.0f,
#define kMatSmooth  0.25f, 1.0f, 0.0f, 0.0f,
#define kMatMetal   0.75f, 1.0f, 1.0f, 0.0f,

static const mat_preset_t ms_matPresets[] =
{
    {
        "wbrick", // wet brick, white brick? wizard brick?
        kMatSmooth
    },
    {
        "brick",
        kMatRough
    },
    {
        "church",
        kMatRough
    },
    {
        "metal",
        kMatMetal
    },
    {
        "quake",
        kMatMetal
    },
    {
        "wmet", // wet metal? white metal? wizard metal?
        kMatMetal
    },
    {
        "wizmet", // wizard metal, my favorite genre
        kMatMetal
    },
    {
        "cop", // copper
        kMatMetal
    },
    {
        "tech", // technology?
        kMatGen
    },
    {
        "city", // ???
        kMatGen
    },
    {
        "rock",
        kMatRough
    },
    {
        "wood",
        kMatGen
    },
    {
        "wizwood", // wizard wood? is that innuendo?
        kMatSmooth
    },
    {
        "ceiling",
        kMatGen
    },
    {
        "sky", // sky texture (purple or light blue stuff)
        1.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    {
        "water",
        0.1f,
        1.0f,
    },
    {
        "rune",
        kMatMetal
    },
    {
        "wall",
        kMatGen
    },
    {
        "floor",
        kMatGen
    },
    {
        "door",
        kMatGen
    },
    {
        "exit",
        kMatMetal
    },
    {
        "skill",
        kMatMetal
    },
    {
        "trigger",
        kMatGen
    },
    {
        "dem",
        kMatGen
    },
    {
        "wiz",
        kMatSmooth
    },
    {
        "m5",
        kMatGen
    },
};

pim_inline float2 VEC_CALL CalcUv(float4 s, float4 t, float4 p)
{
    // u = dot(P.xyz, s.xyz) + s.w
    // v = dot(P.xyz, t.xyz) + t.w
    return f2_v(f4_dot3(p, s) + s.w, f4_dot3(p, t) + t.w);
}

static i32 VEC_CALL FlattenSurface(
    const mmodel_t* model,
    const msurface_t* surface,
    i32** pim_noalias pTris,
    i32** pim_noalias pPolys)
{
    const i32 surfnumedges = surface->numedges;
    const i32 surffirstedge = surface->firstedge;

    const i32 modnumsurfedges = model->numsurfedges;
    const i32* pim_noalias surfedges = model->surfedges;
    const i32 modnumedges = model->numedges;
    const medge_t* pim_noalias edges = model->edges;
    const i32 modnumvertices = model->numvertices;
    const float4* pim_noalias vertices = model->vertices;

    i32* pim_noalias polygon = tmp_realloc(*pPolys, sizeof(polygon[0]) * surfnumedges);
    i32* pim_noalias tris = tmp_realloc(*pTris, sizeof(tris[0]) * surfnumedges * 3);
    *pPolys = polygon;
    *pTris = tris;

    for (i32 i = 0; i < surfnumedges; ++i)
    {
        i32 j = surffirstedge + i;
        ASSERT(j < modnumsurfedges);
        i32 e = surfedges[j];
        i32 v;
        if (e >= 0)
        {
            ASSERT(e < modnumedges);
            v = edges[e].v[0];
        }
        else
        {
            ASSERT(-e < modnumedges);
            v = edges[-e].v[1];
        }
        ASSERT(v >= 0);
        ASSERT(v < modnumvertices);
        polygon[i] = v;
    }

    i32 count = surfnumedges;
    i32 resLen = 0;
    while (count >= 3)
    {
        tris[resLen + 0] = polygon[0];
        tris[resLen + 1] = polygon[1];
        tris[resLen + 2] = polygon[2];
        resLen += 3;
        ASSERT(resLen <= (surfnumedges * 3));

        --count;
        for (i32 j = 1; j < count; ++j)
        {
            polygon[j] = polygon[j + 1];
        }
    }

    return resLen;
}

static i32 FindPreset(const char* name)
{
    const mat_preset_t* presets = ms_matPresets;
    const i32 len = NELEM(ms_matPresets);
    for (i32 i = 0; i < len; ++i)
    {
        if (StrIStr(name, PIM_PATH, presets[i].name))
        {
            return i;
        }
    }
    return -1;
}

static float4 VEC_CALL HueToRgb(float h)
{
    float r = f1_abs(h * 6.0f - 3.0f) - 1.0f;
    float g = 2.0f - f1_abs(h * 6.0f - 2.0f);
    float b = 2.0f - f1_abs(h * 6.0f - 4.0f);
    return f4_saturate(f4_v(r, g, b, 1.0f));
}

static float4 VEC_CALL HsvToRgb(float4 hsv)
{
    float4 rgb = HueToRgb(hsv.x);
    float4 a = f4_subvs(rgb, 1.0f);
    float4 b = f4_mulvs(a, hsv.y);
    float4 c = f4_addvs(b, 1.0f);
    float4 d = f4_mulvs(c, hsv.z);
    return d;
}

static float4 VEC_CALL IntToColor(i32 i, i32 count)
{
    float h = (i + 0.5f) / count;
    for (i32 b = 0; b < 4; ++b)
    {
        i32 m = 1 << b;
        if (i & m)
        {
            h += 0.5f / m;
        }
    }
    h = fmodf(h, 1.0f);
    return HsvToRgb(f4_v(h, 0.75f, 0.9f, 1.0f));
}

static material_t GenMaterial(const mtexture_t* mtex)
{
    material_t material = { 0 };
    material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);

    if (!mtex)
    {
        return material;
    }

    material.ior = 1.0f;
    if (StrIStr(ARGS(mtex->name), "light"))
    {
        material.flags |= matflag_emissive;
    }
    if (StrIStr(ARGS(mtex->name), "sky"))
    {
        material.flags |= matflag_sky;
        material.flags |= matflag_emissive;
    }
    if (StrIStr(ARGS(mtex->name), "lava"))
    {
        material.flags |= matflag_lava;
        material.flags |= matflag_emissive;
        material.flags |= matflag_animated;
    }
    if (StrIStr(ARGS(mtex->name), "water"))
    {
        material.ior = 1.333f;
        material.flags |= matflag_water;
        material.flags |= matflag_refractive;
    }
    if (StrIStr(ARGS(mtex->name), "slime"))
    {
        material.ior = 1.4394f;
        material.flags |= matflag_slime;
        material.flags |= matflag_refractive;
    }
    if (StrIStr(ARGS(mtex->name), "teleport"))
    {
        material.flags |= matflag_emissive;
    }
    if (StrIStr(ARGS(mtex->name), "window"))
    {
        material.ior = 1.52f;
        material.flags |= matflag_refractive;
        material.flags |= matflag_emissive;
    }
    if (mtex->name[0] == '*')
    {
        // uv animated
        material.flags |= matflag_warped;
    }
    if (mtex->name[0] == '+')
    {
        // keyframe animated
        material.flags |= matflag_animated;
    }

    const u8* pim_noalias mip0 = (u8*)mtex + mtex->offsets[0];
    ASSERT(mtex->offsets[0] == sizeof(mtexture_t));
    const int2 size = i2_v(mtex->width, mtex->height);
    const i32 texelCount = size.x * size.y;

    if (!(material.flags & matflag_emissive))
    {
        for (i32 i = 0; i < texelCount; ++i)
        {
            // 224: fire
            // 240: brights
            if (mip0[i] >= 224)
            {
                material.flags |= matflag_emissive;
                break;
            }
        }
    }

    float roughness = 0.5f;
    float occlusion = 1.0f;
    float metallic = 0.0f;
    float emission = 0.0f;

    i32 iPreset = FindPreset(mtex->name);
    if (iPreset != -1)
    {
        roughness = ms_matPresets[iPreset].roughness;
        occlusion = ms_matPresets[iPreset].occlusion;
        metallic = ms_matPresets[iPreset].metallic;
        emission = ms_matPresets[iPreset].emission;
    }

    if (emission > 0.0f)
    {
        material.flags |= matflag_emissive;
    }
    if ((emission == 0.0f) && (material.flags & matflag_emissive))
    {
        emission = 0.5f;
    }

    material.flatAlbedo = LinearToColor(f4_1);
    material.flatRome = LinearToColor(f4_v(roughness, occlusion, metallic, emission));
    texture_unpalette(
        mip0,
        size,
        mtex->name,
        &material.albedo,
        &material.rome,
        &material.normal);

    return material;
}

static mesh_t VEC_CALL TrisToMesh(
    const mmodel_t* model,
    float4x4 M,
    const msurface_t* surface,
    const i32* pim_noalias inds,
    i32 vertCount)
{
    ASSERT((vertCount % 3) == 0);

    const i32 numverts = model->numvertices;
    const float4* pim_noalias verts = model->vertices;

    float4* pim_noalias positions = perm_malloc(sizeof(positions[0]) * vertCount);
    float4* pim_noalias normals = perm_malloc(sizeof(normals[0]) * vertCount);
    float2* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);

    float4 s = f4_0;
    float4 t = f4_0;
    float2 uvScale = f2_0;
    {
        const mtexinfo_t* texinfo = surface->texinfo;
        if (texinfo)
        {
            s = texinfo->vecs[0];
            t = texinfo->vecs[1];
            const mtexture_t* mtex = texinfo->texture;
            if (mtex)
            {
                uvScale = f2_v(1.0f / mtex->width, 1.0f / mtex->height);
            }
        }
    }

    i32 vertsEmit = 0;
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        i32 a = inds[i + 0];
        i32 b = inds[i + 1];
        i32 c = inds[i + 2];
        ASSERT(a >= 0);
        ASSERT(c < numverts);

        float4 A0 = verts[a];
        float4 B0 = verts[b];
        float4 C0 = verts[c];
        float4 A = f4x4_mul_pt(M, A0);
        float4 B = f4x4_mul_pt(M, B0);
        float4 C = f4x4_mul_pt(M, C0);

        float4 N = f4_cross3(f4_sub(C, A), f4_sub(B, A));
        float lenSq = f4_dot3(N, N);
        if (lenSq > kEpsilon)
        {
            N = f4_divvs(N, sqrtf(lenSq));
        }
        else
        {
            // line, discard.
            continue;
        }

        // reverse winding order
        a = vertsEmit + 0;
        b = vertsEmit + 2;
        c = vertsEmit + 1;

        ASSERT(b < vertCount);

        positions[a] = A;
        positions[b] = B;
        positions[c] = C;

        normals[a] = N;
        normals[b] = N;
        normals[c] = N;

        uvs[a] = f2_mul(CalcUv(s, t, A0), uvScale);
        uvs[b] = f2_mul(CalcUv(s, t, B0), uvScale);
        uvs[c] = f2_mul(CalcUv(s, t, C0), uvScale);

        vertsEmit += 3;
    }

    mesh_t mesh = { 0 };
    if (vertsEmit > 0)
    {
        ASSERT(vertsEmit <= vertCount);
        mesh.length = vertsEmit;
        mesh.positions = positions;
        mesh.normals = normals;
        mesh.uvs = uvs;
    }
    else
    {
        pim_free(positions);
        pim_free(normals);
        pim_free(uvs);
    }

    return mesh;
}

static void FixZFighting(mesh_t mesh)
{
    const i32 len = mesh.length;
    const float4* pim_noalias normals = mesh.normals;
    float4* pim_noalias positions = mesh.positions;
    for (i32 i = 0; i < len; ++i)
    {
        float4 P = positions[i];
        float4 N = normals[i];
        P = f4_add(P, f4_mulvs(N, kMilli * 0.5f));
        positions[i] = P;
    }
}

typedef struct batch_s
{
    i32 length;
    const msurface_t* surfaces;
    const mtexinfo_t** texinfos;
    const mtexture_t** textures;
    const char** texnames;
    i32* batchids;
    i32* indices;
} batch_t;

static i32 BatchSortFn(i32 lhs, i32 rhs, void* usr)
{
    const batch_t* batch = usr;

    i32 namecmp = StrCmp(batch->texnames[lhs], 16, batch->texnames[rhs]);
    if (namecmp)
    {
        return namecmp;
    }

    const msurface_t* lsurf = batch->surfaces + lhs;
    const msurface_t* rsurf = batch->surfaces + rhs;

    i32 lflags = lsurf->flags;
    i32 rflags = rsurf->flags;
    if (lflags != rflags)
    {
        return lflags < rflags ? -1 : 1;
    }

    i32 lplane = lsurf->planenum;
    i32 rplane = rsurf->planenum;
    if (lplane != rplane)
    {
        return lplane < rplane ? -1 : 1;
    }

    i32 lside = lsurf->side;
    i32 rside = rsurf->side;
    if (lside != rside)
    {
        return lside < rside ? -1 : 1;
    }

    i32 ledge = lsurf->firstedge;
    i32 redge = rsurf->firstedge;
    if (ledge != redge)
    {
        return ledge < redge ? -1 : 1;
    }

    return 0;
}

static batch_t ModelToBatch(const mmodel_t* model)
{
    batch_t batch = { 0 };

    const i32 len = model->numsurfaces;
    batch.length = len;
    batch.surfaces = model->surfaces;
    batch.texinfos = tmp_malloc(sizeof(batch.texinfos[0]) * len);
    batch.textures = tmp_malloc(sizeof(batch.textures[0]) * len);
    batch.texnames = tmp_malloc(sizeof(batch.texnames[0]) * len);
    batch.batchids = tmp_malloc(sizeof(batch.batchids[0]) * len);
    batch.indices = tmp_malloc(sizeof(batch.indices[0]) * len);

    const i32 numsurfedges = model->numsurfedges;
    for (i32 i = 0; i < len; ++i)
    {
        const msurface_t* surface = batch.surfaces + i;
        const i32 numedges = surface->numedges;
        const i32 firstedge = surface->firstedge;
        const mtexinfo_t* texinfo = surface->texinfo;

        batch.texinfos[i] = texinfo;
        batch.textures[i] = NULL;
        batch.texnames[i] = "null";
        batch.batchids[i] = -1;
        batch.indices[i] = i;

        if ((numedges <= 0) || (firstedge < 0))
        {
            continue;
        }
        if ((firstedge + numedges) > numsurfedges)
        {
            continue;
        }
        if (!texinfo)
        {
            continue;
        }
        const mtexture_t* mtex = texinfo->texture;
        batch.textures[i] = mtex;
        if (!mtex)
        {
            continue;
        }
        batch.texnames[i] = mtex->name;
    }

    sort_i32(batch.indices, len, BatchSortFn, &batch);

    i32 curbatch = 0;
    if (len > 0)
    {
        i32 j = batch.indices[0];
        batch.batchids[j] = curbatch;
    }
    for (i32 i = 1; i < len; ++i)
    {
        i32 prev = batch.indices[i - 1];
        i32 cur = batch.indices[i];
        if (StrCmp(batch.texnames[prev], 16, batch.texnames[cur]))
        {
            ++curbatch;
        }
        batch.batchids[cur] = curbatch;
    }

    return batch;
}

void ModelToDrawables(const mmodel_t* model)
{
    ASSERT(model);
    ASSERT(model->vertices);

    const quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    const float4x4 M = f4x4_trs(f4_0, rot, f4_s(0.02f));

    const char* name = model->name;
    const i32 numsurfaces = model->numsurfaces;
    const msurface_t* surfaces = model->surfaces;
    const i32 numsurfedges = model->numsurfedges;
    const float4* vertices = model->vertices;

    drawables_t* drawTable = drawables_get();
    batch_t batch = ModelToBatch(model);

    i32* polygon = NULL;
    i32* tris = NULL;

    const mtexture_t* batchtex = NULL;
    i32 curbatch = -1;
    mesh_t mesh = { 0 };
    for (i32 i = 0; i < batch.length; ++i)
    {
        const i32 j = batch.indices[i];

        const msurface_t* surface = batch.surfaces + j;
        const i32 numedges = surface->numedges;
        const i32 firstedge = surface->firstedge;
        const mtexinfo_t* texinfo = batch.texinfos[j];
        const mtexture_t* mtex = batch.textures[j];
        const char* texname = batch.texnames[j];
        const i32 batchid = batch.batchids[j];

        if (batchid != curbatch)
        {
            if (mesh.length > 0)
            {
                char name[PIM_PATH];
                SPrintf(ARGS(name), "%s_batch_%d", model->name, curbatch);
                meshid_t meshid;
                if (mesh_new(&mesh, name, &meshid))
                {
                    u32 id = iid_new();
                    i32 c = drawables_add(id);
                    drawTable->meshes[c] = meshid;
                    drawTable->materials[c] = GenMaterial(batchtex);
                    drawTable->translations[c] = f4_0;
                    drawTable->scales[c] = f4_1;
                    drawTable->rotations[c] = quat_id;
                    drawTable->matrices[c] = f4x4_id;
                }
                else
                {
                    ASSERT(false);
                }
            }
            memset(&mesh, 0, sizeof(mesh));
            curbatch = batchid;
            batchtex = mtex;
        }

        if ((numedges <= 0) || (firstedge < 0))
        {
            continue;
        }
        if ((firstedge + numedges) > numsurfedges)
        {
            continue;
        }
        if (StrIStr(texname, 16, "trigger"))
        {
            continue;
        }

        i32 vertCount = FlattenSurface(model, surface, &tris, &polygon);
        mesh_t submesh = TrisToMesh(model, M, surface, tris, vertCount);
        if ((texname[0] == '*') || (texname[0] == '+'))
        {
            FixZFighting(submesh);
        }

        const i32 addlen = submesh.length;
        const i32 back = mesh.length;
        const i32 newlen = back + addlen;
        mesh.length = newlen;
        PermReserve(mesh.positions, newlen);
        PermReserve(mesh.normals, newlen);
        PermReserve(mesh.uvs, newlen);
        for (i32 i = 0; i < addlen; ++i)
        {
            mesh.positions[back + i] = submesh.positions[i];
            mesh.normals[back + i] = submesh.normals[i];
            mesh.uvs[back + i] = submesh.uvs[i];
        }
        pim_free(submesh.positions);
        pim_free(submesh.normals);
        pim_free(submesh.uvs);
        memset(&submesh, 0, sizeof(submesh));
    }
}

bool LoadModelAsDrawables(const char* name)
{
    asset_t asset = { 0 };
    if (asset_get(name, &asset))
    {
        mmodel_t* model = LoadModel(name, asset.pData, EAlloc_Temp);
        ModelToDrawables(model);
        FreeModel(model);
        return true;
    }
    return false;
}
