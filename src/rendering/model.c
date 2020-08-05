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

#include "quake/q_model.h"

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
    float4** pim_noalias pTris,
    float4** pim_noalias pPolys)
{
    i32 numEdges = surface->numedges;
    const i32 firstEdge = surface->firstedge;
    const i32* pim_noalias surfEdges = model->surfedges;
    const medge_t* pim_noalias edges = model->edges;
    const float4* pim_noalias vertices = model->vertices;

    float4* pim_noalias polygon = tmp_realloc(*pPolys, sizeof(polygon[0]) * numEdges);
    *pPolys = polygon;

    for (i32 i = 0; i < numEdges; ++i)
    {
        i32 e = surfEdges[firstEdge + i];
        i32 v;
        if (e >= 0)
        {
            v = edges[e].v[0];
        }
        else
        {
            v = edges[-e].v[1];
        }
        polygon[i] = vertices[v];
    }

    float4* pim_noalias triangles = tmp_realloc(*pTris, sizeof(triangles[0]) * numEdges * 3);
    *pTris = triangles;

    i32 resLen = 0;
    while (numEdges >= 3)
    {
        triangles[resLen + 0] = polygon[0];
        triangles[resLen + 1] = polygon[1];
        triangles[resLen + 2] = polygon[2];
        resLen += 3;

        --numEdges;
        for (i32 j = 1; j < numEdges; ++j)
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

static material_t* GenMaterials(const msurface_t* surfaces, i32 surfCount)
{
    ASSERT(surfaces);

    material_t* materials = tmp_calloc(sizeof(materials[0]) * surfCount);

    for (i32 i = 0; i < surfCount; ++i)
    {
        const msurface_t* surface = surfaces + i;
        const mtexinfo_t* texinfo = surface->texinfo;
        if (!texinfo)
        {
            continue;
        }
        const mtexture_t* mtex = texinfo->texture;
        if (!mtex)
        {
            continue;
        }

        material_t material = { 0 };
        material.ior = 1.0f;
        if (StrIStr(ARGS(mtex->name), "light"))
        {
            material.flags |= matflag_emissive;
        }
        if (StrIStr(ARGS(mtex->name), "sky"))
        {
            material.flags |= matflag_sky;
            material.flags |= matflag_emissive;
            material.flags |= matflag_animated;
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
            material.flags |= matflag_refractive;
            material.flags |= matflag_animated;
        }
        if (StrIStr(ARGS(mtex->name), "slime"))
        {
            material.ior = 1.4394f;
            material.flags |= matflag_refractive;
            material.flags |= matflag_animated;
        }
        if (StrIStr(ARGS(mtex->name), "teleport"))
        {
            material.flags |= matflag_emissive;
            material.flags |= matflag_animated;
        }
        if (StrIStr(ARGS(mtex->name), "window"))
        {
            material.ior = 1.52f;
            material.flags |= matflag_refractive;
            material.flags |= matflag_emissive;
        }
        if (mtex->name[0] == '*')
        {
            // 'turbulent'
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
        materials[i] = material;
    }

    return materials;
}

static meshid_t VEC_CALL TrisToMesh(
    const char* name,
    float4x4 M,
    const msurface_t* surface,
    const float4* pim_noalias tris,
    i32 vertCount)
{
    meshid_t id = { 0 };

    const mtexinfo_t* texinfo = surface->texinfo;
    ASSERT(texinfo);
    const mtexture_t* mtex = texinfo->texture;
    ASSERT(mtex);

    float4 s = texinfo->vecs[0];
    float4 t = texinfo->vecs[1];
    float2 uvScale = { 1.0f / mtex->width, 1.0f / mtex->height };

    float4* pim_noalias positions = perm_malloc(sizeof(positions[0]) * vertCount);
    float4* pim_noalias normals = perm_malloc(sizeof(normals[0]) * vertCount);
    float2* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);

    i32 vertsEmit = 0;
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        float4 A0 = tris[i + 0];
        float4 B0 = tris[i + 1];
        float4 C0 = tris[i + 2];
        float4 A = f4x4_mul_pt(M, A0);
        float4 B = f4x4_mul_pt(M, B0);
        float4 C = f4x4_mul_pt(M, C0);

        // reverse winding order
        i32 a = vertsEmit + 0;
        i32 b = vertsEmit + 2;
        i32 c = vertsEmit + 1;

        float4 N = f4_cross3(f4_sub(C, A), f4_sub(B, A));
        float lenSq = f4_dot3(N, N);
        if (lenSq > kEpsilon)
        {
            vertsEmit += 3;
            N = f4_divvs(N, sqrtf(lenSq));
        }
        else
        {
            // line, discard.
            continue;
        }

        positions[a] = A;
        positions[b] = B;
        positions[c] = C;

        uvs[a] = f2_mul(CalcUv(s, t, A0), uvScale);
        uvs[b] = f2_mul(CalcUv(s, t, B0), uvScale);
        uvs[c] = f2_mul(CalcUv(s, t, C0), uvScale);

        normals[a] = N;
        normals[b] = N;
        normals[c] = N;
    }

    if (vertsEmit > 0)
    {
        mesh_t mesh = { 0 };
        mesh.length = vertsEmit;
        mesh.positions = positions;
        mesh.normals = normals;
        mesh.uvs = uvs;
        bool created = mesh_new(&mesh, name, &id);
        ASSERT(created);
    }
    return id;
}

static void FixZFighting(meshid_t meshid)
{
    mesh_t mesh;
    if (mesh_get(meshid, &mesh))
    {
        for (i32 iVert = 0; iVert <= mesh.length; ++iVert)
        {
            float4 P = mesh.positions[iVert];
            float4 N = mesh.normals[iVert];
            P = f4_add(P, f4_mulvs(N, kMilli * 0.5f));
            mesh.positions[iVert] = P;
        }
    }
}

u32* ModelToDrawables(const mmodel_t* model)
{
    ASSERT(model);
    ASSERT(model->vertices);

    const quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    const float4x4 M = f4x4_trs(f4_0, rot, f4_s(0.02f));

    const char* name = model->name;
    const i32 numSurfaces = model->numsurfaces;
    const msurface_t* surfaces = model->surfaces;
    const i32* surfEdges = model->surfedges;
    const float4* vertices = model->vertices;
    const medge_t* edges = model->edges;

    material_t* materials = GenMaterials(surfaces, numSurfaces);
    drawables_t* drawTable = drawables_get();

    u32 idCount = 0;
    u32* ids = NULL;

    float4* polygon = NULL;
    float4* tris = NULL;
    for (i32 i = 0; i < numSurfaces; ++i)
    {
        const msurface_t* surface = surfaces + i;
        const i32 numEdges = surface->numedges;
        const i32 firstEdge = surface->firstedge;
        const mtexinfo_t* texinfo = surface->texinfo;
        if (!texinfo)
        {
            continue;
        }
        const mtexture_t* mtex = texinfo->texture;
        if (!mtex)
        {
            continue;
        }
        if (numEdges <= 0 || firstEdge < 0)
        {
            continue;
        }
        if (StrIStr(ARGS(mtex->name), "trigger"))
        {
            continue;
        }

        material_t material = materials[i];

        char name[PIM_PATH];
        SPrintf(ARGS(name), "%s_surf_%d", model->name, i);
        i32 vertCount = FlattenSurface(model, surface, &tris, &polygon);
        meshid_t mesh = TrisToMesh(name, M, surface, tris, vertCount);

        material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);

        if (material.flags & matflag_animated)
        {
            FixZFighting(mesh);
        }

        u32 id = iid_new();
        ++idCount;
        PermReserve(ids, idCount + 1);
        ids[idCount] = id;

        i32 c = drawables_add(id);
        drawTable->meshes[c] = mesh;
        drawTable->materials[c] = material;
        drawTable->translations[c] = f4_0;
        drawTable->scales[c] = f4_1;
        drawTable->rotations[c] = quat_id;
        drawTable->matrices[c] = f4x4_id;
    }

    if (ids)
    {
        ids[0] = idCount;
    }

    return ids;
}

u32* LoadModelAsDrawables(const char* name)
{
    asset_t asset = { 0 };
    if (asset_get(name, &asset))
    {
        mmodel_t* model = LoadModel(name, asset.pData, EAlloc_Temp);
        u32* ids = ModelToDrawables(model);
        FreeModel(model);
        return ids;
    }
    return NULL;
}
