#include "rendering/fragment_stage.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "threading/task.h"

#include "math/types.h"
#include "math/float4x4_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/lighting.h"
#include "math/color.h"
#include "math/frustum.h"
#include "math/sdf.h"
#include "math/ambcube.h"

#include "rendering/sampler.h"
#include "rendering/tile.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/camera.h"
#include "rendering/framebuffer.h"
#include "rendering/lights.h"
#include "rendering/cubemap.h"
#include "rendering/spheremap.h"
#include "rendering/drawable.h"

static cvar_t cv_gi_dbg_diff = { cvart_bool, 0, "gi_dbg_diff", "0", "view ambient cube with normal vector" };
static cvar_t cv_gi_dbg_spec = { cvart_bool, 0, "gi_dbg_spec", "0", "view cubemap with reflect vector" };
static cvar_t cv_gi_dbg_diffview = { cvart_bool, 0, "gi_dbg_diffview", "0", "view ambient cube with forward vector" };
static cvar_t cv_gi_dbg_specview = { cvart_bool, 0, "gi_dbg_specview", "0", "view cubemap with forward vector" };
static cvar_t cv_gi_dbg_specmip = { cvart_float, 0, "gi_dbg_specmip", "0", "mip level of cubemap debug views" };
static cvar_t cv_gi_spheremap = { cvart_bool, 0, "gi_spheremap", "0", "use spheremap for specgi" };

typedef struct fragstage_s
{
    task_t task;
    framebuf_t* frontBuf;
    const framebuf_t* backBuf;
} fragstage_t;

typedef struct tile_ctx_s
{
    frus_t frus;
    float4x4 VP;
    float4 tileNormal;
    float4 eye;
    float4 right;
    float4 up;
    float4 fwd;
    float2 slope;
    float2 tileMin;
    float2 tileMax;
    float nearClip;
    float farClip;
    float tileDepth;
} tile_ctx_t;

static BrdfLut ms_lut;
static AmbCube_t ms_ambcube;
static SphereMap ms_sphereMap;

AmbCube_t VEC_CALL AmbCube_Get(void) { return ms_ambcube; };
void VEC_CALL AmbCube_Set(AmbCube_t cube) { ms_ambcube = cube; }

struct SphereMap_s* SphereMap_Get() { return &ms_sphereMap; }

static void EnsureInit(void);
static void SetupTile(tile_ctx_t* ctx, i32 iTile, const framebuf_t* backBuf);
static float4 VEC_CALL TriBounds(float4x4 VP, float4 A, float4 B, float4 C, float2 tileMin, float2 tileMax);
static void VEC_CALL DrawMesh(const tile_ctx_t* ctx, framebuf_t* target, i32 iDraw, const Cubemap* cm);
static i32 VEC_CALL FindBounds(sphere_t self, const sphere_t* pim_noalias others, i32 len);

pim_optimize
static void FragmentStageFn(task_t* task, i32 begin, i32 end)
{
    fragstage_t* stage = (fragstage_t*)task;

    const Cubemaps_t* cubeTable = Cubemaps_Get();
    const i32 cubemapCount = cubeTable->count;
    const Cubemap* pim_noalias convmaps = cubeTable->convmaps;
    const sphere_t* pim_noalias cubeBounds = cubeTable->bounds;

    const drawables_t* drawTable = drawables_get();
    const i32 drawCount = drawTable->count;
    const u64* pim_noalias tileMasks = drawTable->tileMasks;
    const sphere_t* pim_noalias drawBounds = drawTable->bounds;

    tile_ctx_t ctx;
    for (i32 iTile = begin; iTile < end; ++iTile)
    {
        SetupTile(&ctx, iTile, stage->backBuf);

        u64 tilemask = 1;
        tilemask <<= iTile;

        for (i32 iDraw = 0; iDraw < drawCount; ++iDraw)
        {
            if (tileMasks[iDraw] & tilemask)
            {
                i32 iCube = FindBounds(drawBounds[iDraw], cubeBounds, cubemapCount);
                const Cubemap* cm = iCube >= 0 ? convmaps + iCube : NULL;
                DrawMesh(&ctx, stage->frontBuf, iDraw, cm);
            }
        }
    }
}

static void EnsureInit(void)
{
    if (!ms_lut.texels)
    {
        cvar_reg(&cv_gi_dbg_diff);
        cvar_reg(&cv_gi_dbg_spec);
        cvar_reg(&cv_gi_dbg_diffview);
        cvar_reg(&cv_gi_dbg_specview);
        cvar_reg(&cv_gi_dbg_specmip);
        cvar_reg(&cv_gi_spheremap);

        ms_lut = BakeBRDF(i2_s(256), 1024);

        SphereMap_New(&ms_sphereMap, i2_s(256));
    }
}

ProfileMark(pm_FragmentStage, Drawables_Fragment)
task_t* Drawables_Fragment(
    framebuf_t* frontBuf,
    const framebuf_t* backBuf)
{
    ProfileBegin(pm_FragmentStage);
    ASSERT(frontBuf);
    ASSERT(backBuf);

    EnsureInit();

    fragstage_t* task = tmp_calloc(sizeof(*task));
    task->frontBuf = frontBuf;
    task->backBuf = backBuf;
    task_submit((task_t*)task, FragmentStageFn, kTileCount);

    ProfileEnd(pm_FragmentStage);
    return (task_t*)task;
}

pim_optimize
static float4 VEC_CALL TriBounds(float4x4 VP, float4 A, float4 B, float4 C, float2 tileMin, float2 tileMax)
{
    A = f4x4_mul_pt(VP, A);
    B = f4x4_mul_pt(VP, B);
    C = f4x4_mul_pt(VP, C);

    A = f4_divvs(A, A.w);
    B = f4_divvs(B, B.w);
    C = f4_divvs(C, C.w);

    float4 bounds;
    bounds.x = f1_min(A.x, f1_min(B.x, C.x));
    bounds.y = f1_min(A.y, f1_min(B.y, C.y));
    bounds.z = f1_max(A.x, f1_max(B.x, C.x));
    bounds.w = f1_max(A.y, f1_max(B.y, C.y));

    bounds.x = f1_max(bounds.x, tileMin.x);
    bounds.y = f1_max(bounds.y, tileMin.y);
    bounds.z = f1_min(bounds.z, tileMax.x);
    bounds.w = f1_min(bounds.w, tileMax.y);

    return bounds;
}

pim_optimize
static void SetupTile(tile_ctx_t* ctx, i32 iTile, const framebuf_t* backBuf)
{
    camera_t camera;
    camera_get(&camera);

    const int2 tile = GetTile(iTile);
    ctx->nearClip = camera.nearFar.x;
    ctx->farClip = camera.nearFar.y;
    ctx->slope = proj_slope(f1_radians(camera.fovy), kDrawAspect);

    ctx->fwd = quat_fwd(camera.rotation);
    ctx->right = quat_right(camera.rotation);
    ctx->up = quat_up(camera.rotation);
    ctx->eye = camera.position;

    ctx->tileMin = TileMin(tile);
    ctx->tileMax = TileMax(tile);
    ctx->tileDepth = TileDepth(tile, backBuf->depth);
    ctx->tileNormal = proj_dir(ctx->right, ctx->up, ctx->fwd, ctx->slope, f2_lerp(ctx->tileMin, ctx->tileMax, 0.5f));
    ctx->frus = frus_new(ctx->eye, ctx->right, ctx->up, ctx->fwd, ctx->tileMin, ctx->tileMax, ctx->slope, camera.nearFar);

    float4x4 V = f4x4_lookat(ctx->eye, f4_add(ctx->eye, ctx->fwd), ctx->up);
    float4x4 P = f4x4_perspective(f1_radians(camera.fovy), kDrawAspect, ctx->nearClip, ctx->farClip);
    ctx->VP = f4x4_mul(P, V);
}

pim_optimize
static void VEC_CALL DrawMesh(
    const tile_ctx_t* ctx,
    framebuf_t* target,
    i32 iDraw,
    const Cubemap* cm)
{
    const drawables_t* drawTable = drawables_get();

    mesh_t mesh;
    texture_t albedoMap = { 0 };
    texture_t romeMap = { 0 };
    if (!mesh_get(drawTable->tmpMeshes[iDraw], &mesh))
    {
        return;
    }

    const bool dbgdiffGI = cv_gi_dbg_diff.asFloat != 0.0f;
    const bool dbgspecGI = cv_gi_dbg_spec.asFloat != 0.0f;
    const bool dbgdiffviewGI = cv_gi_dbg_diffview.asFloat != 0.0f;
    const bool dbgspecviewGI = cv_gi_dbg_specview.asFloat != 0.0f;
    const bool useSpheremap = cv_gi_spheremap.asFloat != 0.0f;
    const float dbgspecmip = cv_gi_dbg_specmip.asFloat;

    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float e = 1.0f / (1 << 10);

    const BrdfLut lut = ms_lut;
    const material_t material = drawTable->materials[iDraw];
    const float4 flatAlbedo = ColorToLinear(material.flatAlbedo);
    const float4 flatRome = ColorToLinear(material.flatRome);
    texture_get(material.albedo, &albedoMap);
    texture_get(material.rome, &romeMap);

    const float4 eye = ctx->eye;
    const float4 fwd = ctx->fwd;
    const float4 right = ctx->right;
    const float4 up = ctx->up;
    const float2 slope = ctx->slope;
    const float nearClip = ctx->nearClip;
    const float farClip = ctx->farClip;
    const float4 tileNormal = ctx->tileNormal;
    const float2 tileMin = ctx->tileMin;
    const float2 tileMax = ctx->tileMax;
    const float tileDepth = ctx->tileDepth;
    const plane_t fwdPlane = plane_new(fwd, eye);

    const lights_t* lights = lights_get();
    const i32 dirCount = lights->dirCount;
    const dir_light_t* pim_noalias dirLights = lights->dirLights;
    const i32 ptCount = lights->ptCount;
    const pt_light_t* pim_noalias ptLights = lights->ptLights;

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    const float4* pim_noalias positions = mesh.positions;
    const float4* pim_noalias normals = mesh.normals;
    const float2* pim_noalias uvs = mesh.uvs;
    const i32 vertCount = mesh.length;

    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float4 A = positions[iVert + 0];
        const float4 B = positions[iVert + 1];
        const float4 C = positions[iVert + 2];

        const float4 BA = f4_sub(B, A);
        const float4 CA = f4_sub(C, A);

        {
            // backface culling
            if (f4_dot3(tileNormal, f4_cross3(CA, BA)) < 0.0f)
            {
                continue;
            }
            const sphere_t sph = triToSphere(A, B, C);
            // occlusion culling
            if (sdPlaneSphere(fwdPlane, sph) > tileDepth)
            {
                continue;
            }
            // tile-frustum-triangle culling
            if (sdFrusSph(ctx->frus, sph) > 0.0f)
            {
                continue;
            }
        }

        const float4 NA = normals[iVert + 0];
        const float4 NB = normals[iVert + 1];
        const float4 NC = normals[iVert + 2];

        const float2 UA = uvs[iVert + 0];
        const float2 UB = uvs[iVert + 1];
        const float2 UC = uvs[iVert + 2];

        const float4 T = f4_sub(eye, A);
        const float4 Q = f4_cross3(T, BA);
        const float t0 = f4_dot3(CA, Q);

        // bounds is broken by triangle clipping / clip space
        const float4 bounds = TriBounds(ctx->VP, A, B, C, tileMin, tileMax);
        for (float y = bounds.y; y < bounds.w; y += dy)
        {
            for (float x = bounds.x; x < bounds.z; x += dx)
            {
                const float2 coord = { x, y };
                const float4 rd = proj_dir(right, up, fwd, slope, coord);
                const float4 rdXca = f4_cross3(rd, CA);
                const float det = f4_dot3(BA, rdXca);
                if (det < e)
                {
                    continue;
                }

                const i32 iTexel = SnormToIndex(coord);
                float4 wuvt;
                {
                    // barycentric and depth clipping
                    const float rcpDet = 1.0f / det;
                    float t = t0 * rcpDet;
                    if ((t < nearClip) || (t > dstDepth[iTexel]))
                    {
                        continue;
                    }
                    wuvt.y = f4_dot3(T, rdXca) * rcpDet;
                    wuvt.z = f4_dot3(rd, Q) * rcpDet;
                    wuvt.x = 1.0f - wuvt.y - wuvt.z;
                    if ((wuvt.x < 0.0f) || (wuvt.y < 0.0f) || (wuvt.z < 0.0f))
                    {
                        continue;
                    }
                    dstDepth[iTexel] = t;
                }

                float4 lighting = f4_0;
                {
                    // blend interpolators
                    const float4 P = f4_blend(A, B, C, wuvt);
                    const float4 N = f4_normalize3(f4_blend(NA, NB, NC, wuvt));
                    const float2 U = f2_frac(f2_blend(UA, UB, UC, wuvt));

                    // lighting
                    const float4 V = f4_normalize3(f4_sub(eye, P));
                    const float4 R = f4_normalize3(f4_reflect(f4_neg(V), N));
                    float4 albedo = flatAlbedo;
                    if (albedoMap.texels)
                    {
                        albedo = f4_mul(albedo,
                            UvBilinearWrap_c32(albedoMap.texels, albedoMap.size, U));
                    }
                    float4 rome = flatRome;
                    if (romeMap.texels)
                    {
                        rome = f4_mul(rome,
                            UvBilinearWrap_c32(romeMap.texels, romeMap.size, U));
                    }
                    const float mip = Cubemap_Rough2Mip(rome.x);
                    {
                        float4 specularGI = f4_0;
                        float4 diffuseGI = f4_0;

                        diffuseGI = AmbCube_Irradiance(ms_ambcube, N);
                        if (useSpheremap)
                        {
                            specularGI = SphereMap_Read(&ms_sphereMap, R);
                        }
                        else
                        {
                            if (cm)
                            {
                                specularGI = Cubemap_Read(cm, R, mip);
                            }
                        }
                        const float4 indirect = IndirectBRDF(lut, V, N, diffuseGI, specularGI, albedo, rome.x, rome.z, rome.y);
                        lighting = f4_add(lighting, indirect);
                    }
                    for (i32 iLight = 0; iLight < dirCount; ++iLight)
                    {
                        dir_light_t light = dirLights[iLight];
                        const float4 direct = DirectBRDF(V, light.dir, light.rad, N, albedo, rome.x, rome.z);
                        lighting = f4_add(lighting, direct);
                    }
                    for (i32 iLight = 0; iLight < ptCount; ++iLight)
                    {
                        pt_light_t light = ptLights[iLight];
                        float4 dir = f4_sub(light.pos, P);
                        float dist = f4_length3(dir);
                        dir = f4_divvs(dir, dist);
                        float attenuation = 1.0f / (0.01f + dist * dist);
                        light.rad = f4_mulvs(light.rad, attenuation);
                        const float4 direct = DirectBRDF(V, dir, light.rad, N, albedo, rome.x, rome.z);
                        lighting = f4_add(lighting, direct);
                    }

                    if (dbgdiffviewGI)
                    {
                        lighting = AmbCube_Eval(ms_ambcube, f4_neg(V));
                    }
                    else if (dbgspecviewGI)
                    {
                        float4 I = f4_neg(V);
                        if (useSpheremap)
                        {
                            lighting = SphereMap_Read(&ms_sphereMap, I);
                        }
                        else
                        {
                            if (cm)
                            {
                                lighting = Cubemap_Read(cm, I, mip);
                            }
                        }
                    }
                    else if (dbgdiffGI)
                    {
                        lighting = AmbCube_Irradiance(ms_ambcube, N);
                    }
                    else if (dbgspecGI)
                    {
                        if (useSpheremap)
                        {
                            lighting = SphereMap_Read(&ms_sphereMap, R);
                        }
                        else
                        {
                            if (cm)
                            {
                                lighting = Cubemap_Read(cm, R, mip);
                            }
                        }
                    }
                }

                dstLight[iTexel] = lighting;
            }
        }
    }
}

static i32 VEC_CALL FindBounds(sphere_t self, const sphere_t* pim_noalias others, i32 len)
{
    i32 chosen = -1;
    float chosenDist = 1 << 20;
    float4 a = self.value;
    float ar2 = a.w * a.w;
    for (i32 i = 0; i < len; ++i)
    {
        float4 b = others[i].value;
        float br2 = b.w * b.w;
        float dist = f4_distancesq3(a, b) - ar2 - br2;
        if (dist < chosenDist)
        {
            chosen = i;
            chosenDist = dist;
        }
    }
    return chosen;
}
