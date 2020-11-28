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
#include "allocator/allocator.h"
#include "assets/asset_system.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "common/sort.h"
#include "logic/progs.h"
#include "rendering/camera.h"
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
        0.5f,
        1.0f,
    },
    {
        "slip", // slipgate (warped starry void)
        kMatGen
    },
    {
        "teleport", // teleporter (map change, red lights)
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

static material_t GenMaterial(const mtexture_t* mtex, const msurface_t* surf)
{
    material_t material = { 0 };
    material.ior = 1.0f;

    float roughness = 0.5f;
    float occlusion = 1.0f;
    float metallic = 0.0f;
    float emission = 0.0f;

    if (mtex)
    {
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
        }
        if (StrIStr(ARGS(mtex->name), "slime"))
        {
            material.ior = 1.4394f;
            material.flags |= matflag_slime;
            material.flags |= matflag_refractive;
        }
        if (StrIStr(ARGS(mtex->name), "water"))
        {
            material.ior = 1.333f;
            material.flags |= matflag_water;
            material.flags |= matflag_refractive;
        }
        if (StrIStr(ARGS(mtex->name), "window"))
        {
            material.ior = 1.52f;
            material.flags |= matflag_refractive;
            material.flags |= matflag_emissive;
        }
        if (StrIStr(ARGS(mtex->name), "teleport"))
        {
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

        u8 const *const pim_noalias mip0 = (u8*)(mtex + 1);
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

        if (emission > 0.0f)
        {
            material.flags |= matflag_emissive;
        }
        if ((emission == 0.0f) && (material.flags & matflag_emissive))
        {
            emission = 0.5f;
        }

        float4 flatRome = f4_v(roughness, occlusion, metallic, emission);
        texture_unpalette(
            mip0,
            size,
            mtex->name,
            material.flags,
            flatRome,
            &material.albedo,
            &material.rome,
            &material.normal);
    }

    return material;
}

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
    float4* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);

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

        float2 uva = f2_mul(CalcUv(s, t, A0), uvScale);
        float2 uvb = f2_mul(CalcUv(s, t, B0), uvScale);
        float2 uvc = f2_mul(CalcUv(s, t, C0), uvScale);
        uvs[a] = f4_v(uva.x, uva.y, 0.0f, 0.0f);
        uvs[b] = f4_v(uvb.x, uvb.y, 0.0f, 0.0f);
        uvs[c] = f4_v(uvc.x, uvc.y, 0.0f, 0.0f);

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
    const msurface_t* pim_noalias surfaces;
    const mtexture_t** pim_noalias textures;
    const char** pim_noalias texnames;
    i32* pim_noalias batchids;
    i32* pim_noalias indices;
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

static float4x4 QuakeToRhsMeters(void)
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

void ModelToDrawables(const mmodel_t* model)
{
    ASSERT(model);
    ASSERT(model->vertices);

    const float4x4 M = QuakeToRhsMeters();

    const char* name = model->name;
    const i32 numsurfaces = model->numsurfaces;
    const msurface_t* surfaces = model->surfaces;
    const i32 numsurfedges = model->numsurfedges;
    const float4* vertices = model->vertices;

    drawables_t* dr = drawables_get();
    batch_t batch = ModelToBatch(model);

    i32* polygon = NULL;
    i32* tris = NULL;

    const mtexture_t* batchtex = NULL;
    const msurface_t* batchsurf = NULL;
    i32 curbatch = -1;
    mesh_t mesh = { 0 };
    for (i32 i = 0; i < batch.length; ++i)
    {
        const i32 j = batch.indices[i];

        const msurface_t* surface = batch.surfaces + j;
        const i32 numedges = surface->numedges;
        const i32 firstedge = surface->firstedge;
        const mtexture_t* mtex = batch.textures[j];
        const char* texname = batch.texnames[j];
        const i32 batchid = batch.batchids[j];

        if (batchid != curbatch)
        {
            if (mesh.length > 0)
            {
                char name[PIM_PATH];
                SPrintf(ARGS(name), "%s_batch_%d", model->name, curbatch);
                guid_t guid = guid_str(name);
                meshid_t meshid;
                if (mesh_new(&mesh, guid, &meshid))
                {
                    i32 c = drawables_add(dr, guid);
                    dr->meshes[c] = meshid;
                    dr->materials[c] = GenMaterial(batchtex, batchsurf);
                    dr->translations[c] = f4_0;
                    dr->scales[c] = f4_1;
                    dr->rotations[c] = quat_id;
                    dr->matrices[c] = f4x4_id;
                }
                else
                {
                    ASSERT(false);
                }
            }
            memset(&mesh, 0, sizeof(mesh));
            curbatch = batchid;
            batchtex = mtex;
            batchsurf = surface;
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

    if (mesh.length > 0)
    {
        char name[PIM_PATH];
        SPrintf(ARGS(name), "%s_batch_%d", model->name, curbatch);
        guid_t guid = guid_str(name);
        meshid_t meshid;
        if (mesh_new(&mesh, guid, &meshid))
        {
            i32 c = drawables_add(dr, guid);
            dr->meshes[c] = meshid;
            dr->materials[c] = GenMaterial(batchtex, batchsurf);
            dr->translations[c] = f4_0;
            dr->scales[c] = f4_1;
            dr->rotations[c] = quat_id;
            dr->matrices[c] = f4x4_id;
        }
        else
        {
            ASSERT(false);
        }
    }
    memset(&mesh, 0, sizeof(mesh));
}

void LoadProgs(const mmodel_t* model, bool loadlights)
{
    progs_t progs = { 0 };
    progs_parse(&progs, model->entities);

    lights_clear();
    const float4x4 M = QuakeToRhsMeters();
    const i32 numentities = progs.numentities;
    const pr_entity_t* entities = progs.entities;
    for (i32 i = 0; i < numentities; ++i)
    {
        pr_entity_t ent = entities[i];
        if ((ent.type >= pr_classname_light) && (ent.type <= pr_classname_light_torch_small_walltorch))
        {
            if (loadlights)
            {
                float rad = ent.light.light * 0.01f;
                pt_light_t pt = { 0 };
                pt.pos = f3_f4(ent.light.origin, 1.0f);
                pt.pos = f4x4_mul_pt(M, pt.pos);
                pt.pos.w = PowerToAttRadius(rad, 0.01f);
                pt.rad = f4_s(rad);
                pt.rad.w = 10.0f * kCenti;
                lights_add_pt(pt);
            }
        }
        else if (ent.type == pr_classname_worldspawn)
        {

        }
        else if (ent.type == pr_classname_info_player_start)
        {
            float4 pt = f3_f4(ent.playerstart.origin, 1.0f);
            float yaw = ent.playerstart.angle;
            quat rot = quat_angleaxis(yaw, f4_y);
            pt = f4x4_mul_pt(M, pt);
            camera_t camera;
            camera_get(&camera);
            camera.position = pt;
            camera.rotation = rot;
            camera_set(&camera);
        }
    }

    progs_del(&progs);
}

bool LoadModelAsDrawables(const char* name, bool loadlights)
{
    asset_t asset = { 0 };
    if (asset_get(name, &asset))
    {
        mmodel_t* model = LoadModel(name, asset.pData, EAlloc_Temp);
        ModelToDrawables(model);
        LoadProgs(model, loadlights);
        FreeModel(model);
        return true;
    }
    return false;
}
