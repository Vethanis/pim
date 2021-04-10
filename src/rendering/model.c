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
#include "math/blending.h"

#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/drawable.h"
#include "rendering/lights.h"
#include "rendering/camera.h"

#include "allocator/allocator.h"
#include "assets/asset_system.h"

#include "common/stringutil.h"
#include "common/console.h"
#include "common/sort.h"
#include "common/fnv1a.h"
#include "common/cvars.h"

#include "quake/q_model.h"
#include "stb/stb_image.h"

#include <string.h>

typedef struct mat_preset_s
{
    char const *const name;
    float roughness;
    float occlusion;
    float metallic;
    float emission;
} mat_preset_t;

#define kMatGen             0.5f, 1.0f, 0.0f, 0.0f,
#define kMatRough           0.75f, 1.0f, 0.0f, 0.0f,
#define kMatSmooth          0.25f, 1.0f, 0.0f, 0.0f,
#define kMatMetal           0.5f, 1.0f, 1.0f, 0.0f,
#define kMatSMetal          0.15f, 1.0f, 1.0f, 0.0f,
#define kMatRMetal          0.75f, 1.0f, 1.0f, 0.0f,

static const mat_preset_t ms_matPresets[] =
{
    {
        "wbrick", // wet brick, white brick? wizard brick?
        kMatRough
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
        kMatSMetal
    },
    {
        "tech", // technology?
        kMatRMetal
    },
    {
        "city", // ???
        kMatRough
    },
    {
        "rock",
        kMatGen
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
        kMatRough
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
        "slime",
        0.2f,
        1.0f,
    },
    {
        "slip", // slipgate (map change, red lights)
        kMatGen
    },
    {
        "teleport", // teleporter (warped starry void)
        kMatMetal
    },
    {
        "rune",
        kMatMetal
    },
    {
        "wall",
        kMatRough
    },
    {
        "floor",
        kMatRough
    },
    {
        "door",
        kMatRough
    },
    {
        "exit",
        kMatSMetal
    },
    {
        "skill",
        kMatMetal
    },
    {
        "wiz",
        kMatSmooth
    },
};

static i32 FindPreset(char const *const name)
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

static Material GenMaterial(
    mtexture_t const *const mtex)
{
    Material material = { 0 };
    material.ior = 1.0f;
    material.bumpiness = ConVar_GetFloat(&cv_r_bumpiness);
    if (!mtex)
    {
        return material;
    }

    float roughness = 0.5f;
    float occlusion = 1.0f;
    float metallic = 0.0f;
    float emission = 0.0f;

    i32 iPreset = FindPreset(mtex->name);
    if (iPreset >= 0)
    {
        roughness = ms_matPresets[iPreset].roughness;
        occlusion = ms_matPresets[iPreset].occlusion;
        metallic = ms_matPresets[iPreset].metallic;
        emission = ms_matPresets[iPreset].emission;
    }

    if (StrIStr(ARGS(mtex->name), "light"))
    {
        material.flags |= MatFlag_Emissive;
    }
    if (StrIStr(ARGS(mtex->name), "sky"))
    {
        material.flags |= MatFlag_Sky;
        material.flags |= MatFlag_Emissive;
    }
    if (StrIStr(ARGS(mtex->name), "lava"))
    {
        material.flags |= MatFlag_Lava;
        material.flags |= MatFlag_Emissive;
    }
    if (StrIStr(ARGS(mtex->name), "slime"))
    {
        material.ior = 1.4394f;
        material.flags |= MatFlag_Slime;
        material.flags |= MatFlag_Refractive;
    }
    if (StrIStr(ARGS(mtex->name), "water"))
    {
        material.ior = 1.333f;
        material.flags |= MatFlag_Water;
        material.flags |= MatFlag_Refractive;
    }
    if (StrIStr(ARGS(mtex->name), "window"))
    {
        material.ior = 1.52f;
        material.flags |= MatFlag_Refractive;
        material.flags |= MatFlag_Emissive;
        roughness = 0.05f;
    }
    if (StrIStr(ARGS(mtex->name), "teleport"))
    {
        material.flags |= MatFlag_Portal;
    }
    if (StrIStr(ARGS(mtex->name), "slip"))
    {
        material.flags |= MatFlag_Emissive;
    }
    if (mtex->name[0] == '*')
    {
        // uv animated
        material.flags |= MatFlag_Warped;
    }
    if (mtex->name[0] == '+')
    {
        // keyframe animated
        material.flags |= MatFlag_Animated;
    }

    u8 const *const pim_noalias mip0 = (u8*)(mtex + 1);
    const int2 size = i2_v(mtex->width, mtex->height);
    const i32 texelCount = size.x * size.y;

    if (!(material.flags & MatFlag_Emissive))
    {
        for (i32 i = 0; i < texelCount; ++i)
        {
            // 224: fire
            // 240: brights
            if (mip0[i] >= 224)
            {
                material.flags |= MatFlag_Emissive;
                break;
            }
        }
    }

    if (emission > 0.0f)
    {
        material.flags |= MatFlag_Emissive;
    }
    if ((emission == 0.0f) && (material.flags & MatFlag_Emissive))
    {
        emission = 0.5f;
    }

    enum
    {
        texslot_albedo = 0,
        texslot_rome,
        texslot_normal,

        texslot_COUNT
    };

    char names[texslot_COUNT][PIM_PATH] = { 0 };
    Guid guids[texslot_COUNT] = { 0 };
    TextureId ids[texslot_COUNT] = { 0 };
    SPrintf(ARGS(names[texslot_albedo]), "%s_albedo", mtex->name);
    SPrintf(ARGS(names[texslot_rome]), "%s_rome", mtex->name);
    SPrintf(ARGS(names[texslot_normal]), "%s_normal", mtex->name);

    for (i32 i = 0; i < NELEM(ids); ++i)
    {
        guids[i] = Guid_FromStr(names[i]);
        if (Texture_Find(guids[i], &ids[i]))
        {
            Texture_Retain(ids[i]);
        }
    }

    if (ConVar_GetBool(&cv_r_tex_custom))
    {
        for (i32 i = 0; i < NELEM(ids); ++i)
        {
            if (Texture_Exists(ids[i]))
            {
                continue;
            }
            char path[PIM_PATH];
            SPrintf(ARGS(path), "data/id1/textures/%s.png", names[i]);
            Texture tex = { 0 };
            i32 comp = 0;
            VkFormat format = 0;
            switch (i)
            {
            default:
                ASSERT(false);
                break;
            case texslot_albedo:
            case texslot_rome:
                format = VK_FORMAT_R8G8B8A8_SRGB;
                tex.texels = stbi_load(path, &tex.size.x, &tex.size.y, &comp, 4);
                break;
            case texslot_normal:
                format = VK_FORMAT_R16G16_SNORM;
                tex.texels = stbi_load_16(path, &tex.size.x, &tex.size.y, &comp, 4);
                break;
            }
            if (tex.texels)
            {
                TextureId customId = { 0 };
                if (Texture_New(&tex, format, guids[i], &customId))
                {
                    Texture_Release(ids[i]);
                    ids[i] = customId;
                }
            }
        }
    }

    bool hasAll = true;
    for (i32 i = 0; i < NELEM(ids); ++i)
    {
        hasAll &= Texture_Exists(ids[i]);
    }
    if (!hasAll)
    {
        TextureId baseIds[3] = { 0 };
        hasAll = Texture_Unpalette(
            mip0,
            size,
            mtex->name,
            &material,
            f4_v(roughness, occlusion, metallic, emission),
            &baseIds[0],
            &baseIds[1],
            &baseIds[2]);
        for (i32 i = 0; i < NELEM(ids); ++i)
        {
            if (!Texture_Exists(ids[i]))
            {
                ids[i] = baseIds[i];
            }
            else
            {
                Texture_Release(baseIds[i]);
            }
        }
    }

    material.albedo = ids[0];
    material.rome = ids[1];
    material.normal = ids[2];

    return material;
}

pim_inline float2 VEC_CALL CalcUv(float4 s, float4 t, float4 p)
{
    // u = dot(P.xyz, s.xyz) + s.w
    // v = dot(P.xyz, t.xyz) + t.w
    return f2_v(f4_dot3(p, s) + s.w, f4_dot3(p, t) + t.w);
}

static i32 FlattenSurface(
    mmodel_t const *const model,
    msurface_t const *const surface,
    i32 **const pim_noalias pTris,
    i32 **const pim_noalias pPolys)
{
    const i32 surfnumedges = surface->numedges;
    const i32 surffirstedge = surface->firstedge;

    const i32 modnumsurfedges = model->numsurfedges;
    i32 const *const pim_noalias surfedges = model->surfedges;
    const i32 modnumedges = model->numedges;
    medge_t const *const pim_noalias edges = model->edges;
    const i32 modnumvertices = model->numvertices;
    float4 const *const pim_noalias vertices = model->vertices;

    *pPolys = Temp_Realloc(*pPolys, sizeof(pPolys[0][0]) * surfnumedges);
    *pTris = Temp_Realloc(*pTris, sizeof(pTris[0][0]) * surfnumedges * 3);
    i32 *const pim_noalias polygon = *pPolys;
    i32 *const pim_noalias tris = *pTris;

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

static void AssignMaterial(Mesh *const mesh, Material material)
{
    const i32 len = mesh->length;
    if (len > 0)
    {
        int4 texIndex = { 0, 0, 0, 0 };
        {
            const Texture* tex = Texture_Get(material.albedo);
            if (tex)
            {
                texIndex.x = tex->slot.index;
            }
            tex = Texture_Get(material.rome);
            if (tex)
            {
                texIndex.y = tex->slot.index;
            }
            tex = Texture_Get(material.normal);
            if (tex)
            {
                texIndex.z = tex->slot.index;
            }
        }
        int4 *const pim_noalias texIndices = Perm_Calloc(sizeof(texIndices[0]) * len);
        mesh->texIndices = texIndices;
        for (i32 i = 0; i < len; ++i)
        {
            texIndices[i] = texIndex;
        }
    }
}

static Mesh VEC_CALL TrisToMesh(
    mmodel_t const *const model,
    float4x4 M,
    msurface_t const *const surface,
    i32 const *const pim_noalias inds,
    i32 indexCount)
{
    ASSERT((indexCount % 3) == 0);
    ASSERT(indexCount > 0);

    float4 s = f4_v(1.0f, 0.0f, 0.0f, 0.0f);
    float4 t = f4_v(0.0f, 1.0f, 0.0f, 0.0f);
    float2 uvScale = f2_1;
    {
        mtexinfo_t const *const texinfo = surface->texinfo;
        if (texinfo)
        {
            s = texinfo->vecs[0];
            t = texinfo->vecs[1];
            mtexture_t const *const mtex = texinfo->texture;
            if (mtex)
            {
                uvScale = f2_v(1.0f / mtex->width, 1.0f / mtex->height);
            }
        }
    }

    float4 *const pim_noalias positions = Perm_Alloc(sizeof(positions[0]) * indexCount);
    float4 *const pim_noalias normals = Perm_Alloc(sizeof(normals[0]) * indexCount);
    float4 *const pim_noalias uvs = Perm_Alloc(sizeof(uvs[0]) * indexCount);

    const i32 modelVertCount = model->numvertices;
    float4 const *const pim_noalias modelVerts = model->vertices;

    i32 vertsEmit = 0;
    for (i32 i = 0; (i + 3) <= indexCount; i += 3)
    {
        i32 a = inds[i + 0];
        i32 b = inds[i + 1];
        i32 c = inds[i + 2];
        ASSERT(a >= 0);
        ASSERT(c < modelVertCount);

        float4 A0 = modelVerts[a];
        float4 B0 = modelVerts[b];
        float4 C0 = modelVerts[c];
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

        ASSERT(b < indexCount);

        positions[a] = A;
        positions[b] = B;
        positions[c] = C;

        normals[a] = N;
        normals[b] = N;
        normals[c] = N;

        float2 uva = f2_mul(CalcUv(s, t, A0), uvScale);
        float2 uvb = f2_mul(CalcUv(s, t, B0), uvScale);
        float2 uvc = f2_mul(CalcUv(s, t, C0), uvScale);
        uvs[a] = f4_v(uva.x, uva.y, 0.0f, 0.0f);
        uvs[b] = f4_v(uvb.x, uvb.y, 0.0f, 0.0f);
        uvs[c] = f4_v(uvc.x, uvc.y, 0.0f, 0.0f);

        vertsEmit += 3;
    }

    Mesh mesh = { 0 };
    if (vertsEmit > 0)
    {
        ASSERT(vertsEmit <= indexCount);
        mesh.length = vertsEmit;
        mesh.positions = positions;
        mesh.normals = normals;
        mesh.uvs = uvs;
    }
    else
    {
        Mem_Free(positions);
        Mem_Free(normals);
        Mem_Free(uvs);
    }

    return mesh;
}

static void FixZFighting(Mesh mesh)
{
    const i32 len = mesh.length;
    float4 const *const pim_noalias normals = mesh.normals;
    float4 *const pim_noalias positions = mesh.positions;
    for (i32 i = 0; i < len; ++i)
    {
        positions[i] = f4_add(positions[i], f4_mulvs(normals[i], kMilli * 0.5f));
    }
}

static float4x4 VEC_CALL QuakeToRhsMeters(void)
{
    // comparative scale metrics via player:
    // quake player is 32x32x48 units
    // default runspeed is 320 units per second
    // max jump is 32 units
    const float heightScale = 1.754f / 48.0f;
    const float speedScale = (1609.34f / (60.0f * 10.0f)) / 320.0f;
    const float jumpScale = 0.508f / 32.0f;
    const float avgScale = (heightScale + speedScale + jumpScale) / 3.0f;
    const quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    const float4x4 M = f4x4_trs(f4_0, rot, f4_s(avgScale));
    return M;
}

static void MergeMesh(
    Mesh *const dst,
    Mesh *const src)
{
    i32 back = dst->length;
    i32 addlen = src->length;
    i32 newlen = back + src->length;
    dst->length = newlen;
    Perm_Reserve(dst->positions, newlen);
    Perm_Reserve(dst->normals, newlen);
    Perm_Reserve(dst->uvs, newlen);
    memcpy(dst->positions + back, src->positions, addlen * sizeof(dst->positions[0]));
    memcpy(dst->normals + back, src->normals, addlen * sizeof(dst->normals[0]));
    memcpy(dst->uvs + back, src->uvs, addlen * sizeof(dst->uvs[0]));
    Mem_Free(src->positions);
    Mem_Free(src->normals);
    Mem_Free(src->uvs);
    memset(src, 0, sizeof(*src));
}

static Guid CreateDrawable(
    Entities *const dr,
    Mesh *const mesh,
    const char* modelName,
    i32 surfIndex,
    const mtexture_t* mtex)
{
    if (mesh->length <= 0)
    {
        return (Guid) { 0 };
    }
    const char* texname = mtex ? mtex->name : "null";
    if ((texname[0] == '*') || (texname[0] == '+'))
    {
        FixZFighting(*mesh);
    }

    Material mat = GenMaterial(mtex);
    AssignMaterial(mesh, mat);

    char name[PIM_PATH] = { 0 };
    SPrintf(ARGS(name), "%s:%d:%s", modelName, surfIndex, texname);
    Guid guid = Guid_FromStr(name);

    MeshId meshid = { 0 };
    if (Mesh_New(mesh, guid, &meshid))
    {
        i32 c = Entities_Add(dr, guid);
        dr->meshes[c] = meshid;
        dr->materials[c] = mat;
        dr->translations[c] = f4_0;
        dr->scales[c] = f4_1;
        dr->rotations[c] = quat_id;
        dr->matrices[c] = f4x4_id;
    }
    else
    {
        ASSERT(false);
    }
    return guid;
}

static i32 CmpName(const void* lhs, const void* rhs, void* usr)
{
    u64 a = *(u64*)lhs;
    u64 b = *(u64*)rhs;
    if (a == b)
    {
        return 0;
    }
    return a < b ? -1 : 1;
}

void ModelSys_Init(void)
{

}

void ModelSys_Update(void)
{

}

void ModelSys_Shutdown(void)
{

}

void ModelToDrawables(mmodel_t const *const model, Entities *const dr)
{
    ASSERT(model);

    const float4x4 M = QuakeToRhsMeters();

    const i32 numsurfaces = model->numsurfaces;
    msurface_t const *const surfaces = model->surfaces;
    const i32 numsurfedges = model->numsurfedges;

    u64 *const pim_noalias hashes = Temp_Calloc(sizeof(hashes[0]) * numsurfaces);
    for (i32 i = 0; i < numsurfaces; ++i)
    {
        const mtexinfo_t* texinfo = surfaces[i].texinfo;
        if (texinfo)
        {
            const mtexture_t* mtex = texinfo->texture;
            if (mtex)
            {
                u64 hash = Fnv64String(mtex->name, Fnv64Bias);
                hash = hash ? hash : 1;
                hashes[i] = hash;
            }
        }
    }
    i32 *const pim_noalias order = IndexSort(hashes, numsurfaces, sizeof(hashes[0]), CmpName, NULL);

    i32* polygon = NULL;
    i32* tris = NULL;
    u64 prevHash = 0;
    Mesh prevMesh = { 0 };
    const mtexture_t* prevTex = NULL;
    i32 prevSurf = -1;
    for (i32 i = 0; i < numsurfaces; ++i)
    {
        i32 j = order[i];
        msurface_t const *const surface = &surfaces[j];
        mtexinfo_t const *const texinfo = surface->texinfo;
        mtexture_t const *const mtex = texinfo ? texinfo->texture : NULL;
        const i32 numedges = surface->numedges;
        const i32 firstedge = surface->firstedge;

        if ((numedges <= 0) || (firstedge < 0))
        {
            continue;
        }
        if ((firstedge + numedges) > numsurfedges)
        {
            continue;
        }
        if (mtex && StrIStr(ARGS(mtex->name), "trigger"))
        {
            continue;
        }

        i32 vertCount = FlattenSurface(model, surface, &tris, &polygon);
        if (vertCount <= 0)
        {
            continue;
        }

        Mesh mesh = TrisToMesh(model, M, surface, tris, vertCount);
        if (hashes[j] == prevHash)
        {
            MergeMesh(&prevMesh, &mesh);
        }
        else
        {
            CreateDrawable(dr, &prevMesh, model->name, prevSurf, prevTex);
            prevHash = hashes[j];
            prevMesh = mesh;
            prevSurf = j;
            prevTex = mtex;
        }
    }

    CreateDrawable(dr, &prevMesh, model->name, prevSurf, prevTex);
    ASSERT(!prevMesh.positions);
}

bool LoadModelAsDrawables(const char* name, Entities *const dr)
{
    asset_t asset = { 0 };
    if (Asset_Get(name, &asset))
    {
        mmodel_t* model = LoadModel(name, asset.pData, EAlloc_Perm);
        ModelToDrawables(model, dr);
        FreeModel(model);
        return true;
    }
    return false;
}
