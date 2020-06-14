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
#include "math/sampling.h"

#include "rendering/sampler.h"
#include "rendering/tile.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/camera.h"
#include "rendering/framebuffer.h"
#include "rendering/lights.h"
#include "rendering/cubemap.h"
#include "rendering/drawable.h"
#include "rendering/lightmap.h"

static cvar_t cv_gi_dbg_diff = { cvart_bool, 0, "gi_dbg_diff", "0", "view lightmap diffuse" };
static cvar_t cv_gi_dbg_spec = { cvart_bool, 0, "gi_dbg_spec", "0", "view lightmap specular" };
static cvar_t cv_gi_dbg_norm = { cvart_bool, 0, "gi_dbg_norm", "0", "view lightmap normal" };
static cvar_t cv_r_uv = { cvart_bool, 0, "r_uv", "0", "view texture uvs" };
static cvar_t cv_r_lm_denoised = { cvart_bool, 0, "r_lm_denoised", "0", "render denoised lightmaps" };

typedef struct fragstage_s
{
    task_t task;
    framebuf_t* frontBuf;
    const framebuf_t* backBuf;
} fragstage_t;

typedef struct tile_ctx_s
{
    frus_t frus;
    float4x4 V;
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
} tile_ctx_t;

static bool ms_once;
static AmbCube_t ms_ambcube;

AmbCube_t VEC_CALL AmbCube_Get(void) { return ms_ambcube; };
void VEC_CALL AmbCube_Set(AmbCube_t cube) { ms_ambcube = cube; }

static void EnsureInit(void);
static void SetupTile(tile_ctx_t* ctx, i32 iTile, const framebuf_t* backBuf);
static void VEC_CALL DrawMesh(const tile_ctx_t* ctx, framebuf_t* target, i32 iDraw, const Cubemap* cm);
static i32 VEC_CALL FindBounds(sphere_t self, const sphere_t* pim_noalias others, i32 len);

pim_optimize
static void FragmentStageFn(task_t* task, i32 begin, i32 end)
{
    fragstage_t* stage = (fragstage_t*)task;

    const Cubemaps_t* cubeTable = Cubemaps_Get();
    const i32 cubemapCount = cubeTable->count;
    const Cubemap* pim_noalias cubemaps = cubeTable->cubemaps;
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
                //i32 iCube = FindBounds(drawBounds[iDraw], cubeBounds, cubemapCount);
                //const Cubemap* cm = iCube >= 0 ? cubemaps + iCube : NULL;
                DrawMesh(&ctx, stage->frontBuf, iDraw, NULL);
            }
        }
    }
}

static void EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;
        cvar_reg(&cv_gi_dbg_diff);
        cvar_reg(&cv_gi_dbg_spec);
        cvar_reg(&cv_gi_dbg_norm);
        cvar_reg(&cv_r_uv);
        cvar_reg(&cv_r_lm_denoised);
    }
}

ProfileMark(pm_FragmentStage, drawables_fragment)
void drawables_fragment(
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
    task_run(&task->task, FragmentStageFn, kTileCount);

    ProfileEnd(pm_FragmentStage);
}

pim_optimize
static float4 VEC_CALL TriBounds(
    float4x4 V,
    float2 slope,
    float zNear, float zFar,
    float4 A, float4 B, float4 C,
    float2 tileMin, float2 tileMax)
{
    A = f4x4_mul_pt(V, A);
    B = f4x4_mul_pt(V, B);
    C = f4x4_mul_pt(V, C);

    float4 scale = { 1.0f / slope.x, 1.0f / slope.y, 1.0f, 1.0f };
    A = f4_mul(A, scale);
    B = f4_mul(B, scale);
    C = f4_mul(C, scale);
    A.z = f1_max(-A.z, f32_eps);
    B.z = f1_max(-B.z, f32_eps);
    C.z = f1_max(-C.z, f32_eps);
    A = f4_divvs(A, A.z);
    B = f4_divvs(B, B.z);
    C = f4_divvs(C, C.z);

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
    ctx->tileNormal = proj_dir(ctx->right, ctx->up, ctx->fwd, ctx->slope, f2_lerp(ctx->tileMin, ctx->tileMax, 0.5f));
    ctx->frus = frus_new(ctx->eye, ctx->right, ctx->up, ctx->fwd, ctx->tileMin, ctx->tileMax, ctx->slope, camera.nearFar);

    float4x4 V = f4x4_lookat(ctx->eye, f4_add(ctx->eye, ctx->fwd), ctx->up);
    ctx->V = V;
}

pim_optimize
static void VEC_CALL DrawMesh(
    const tile_ctx_t* ctx,
    framebuf_t* target,
    i32 iDraw,
    const Cubemap* cm)
{
    const drawables_t* drawTable = drawables_get();
    const lmpack_t* lmpack = lmpack_get();

    mesh_t mesh;
    if (!mesh_get(drawTable->tmpMeshes[iDraw], &mesh))
    {
        return;
    }

    const bool dbgnormGI = cv_gi_dbg_norm.asFloat != 0.0f;
    const bool dbgdiffGI = (cv_gi_dbg_diff.asFloat != 0.0f) || dbgnormGI;
    const bool dbgspecGI = cv_gi_dbg_spec.asFloat != 0.0f;
    const bool dbgUv = cv_r_uv.asFloat != 0.0f;
    const bool denoisedLM = (cv_r_lm_denoised.asFloat != 0.0f) && !dbgnormGI;

    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float e = 1.0f / (1 << 10);

    const material_t material = drawTable->materials[iDraw];
    const float4 flatAlbedo = ColorToLinear(material.flatAlbedo);
    const float4 flatRome = ColorToLinear(material.flatRome);
    texture_t albedoMap = { 0 };
    texture_get(material.albedo, &albedoMap);
    texture_t romeMap = { 0 };
    texture_get(material.rome, &romeMap);
    texture_t normalMap = { 0 };
    texture_get(material.normal, &normalMap);

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

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    const float4* pim_noalias positions = mesh.positions;
    const float4* pim_noalias normals = mesh.normals;
    const float2* pim_noalias uvs = mesh.uvs;
    const float3* pim_noalias lmUvs = mesh.lmUvs;
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
            // tile-frustum-triangle culling
            if (sdFrusSph(ctx->frus, sph) > 0.0f)
            {
                continue;
            }
        }

        const float4 T = f4_sub(eye, A);
        const float4 Q = f4_cross3(T, BA);
        const float t0 = f4_dot3(CA, Q);

        const float4 bounds = TriBounds(ctx->V, slope, nearClip, farClip, A, B, C, tileMin, tileMax);
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
                    const float2 U = f2_frac(f2_blend(uvs[iVert + 0], uvs[iVert + 1], uvs[iVert + 2], wuvt));
                    float4 P = f4_blend(A, B, C, wuvt);
                    float4 N = f4_normalize3(f4_blend(normals[iVert + 0], normals[iVert + 1], normals[iVert + 2], wuvt));
                    float4 V = f4_normalize3(f4_sub(eye, P));
                    float4 R = f4_reflect3(f4_neg(V), N);

                    float4 rome = flatRome;
                    float4 albedo = flatAlbedo;
                    if (albedoMap.texels)
                    {
                        albedo = f4_mul(albedo,
                            UvBilinearWrap_c32(albedoMap.texels, albedoMap.size, U));
                    }

                    if (romeMap.texels)
                    {
                        rome = f4_mul(rome,
                            UvBilinearWrap_c32(romeMap.texels, romeMap.size, U));
                    }

                    if (normalMap.texels)
                    {
                        float4 tN = UvBilinearWrap_dir8(normalMap.texels, normalMap.size, U);
                        N = TanToWorld(N, tN);
                    }

                    {
                        float4 emission = UnpackEmission(albedo, rome.w);
                        lighting = f4_add(lighting, emission);
                    }

                    {
                        float4 diffuseGI = f4_s(0.1f);
                        i32 lmIndex = (i32)lmUvs[iVert + 0].z;
                        ASSERT(lmIndex < lmpack->lmCount);
                        if (lmIndex >= 0)
                        {
                            const float3 lmUv3 = f3_blend(lmUvs[iVert + 0], lmUvs[iVert + 1], lmUvs[iVert + 2], wuvt);
                            const float2 lmUv = { lmUv3.x, lmUv3.y };
                            const lightmap_t lmap = lmpack->lightmaps[lmIndex];
                            const float3* pim_noalias lmBuffer = lmap.color;
                            if (denoisedLM)
                            {
                                lmBuffer = lmap.denoised;
                            }
                            else if (dbgnormGI)
                            {
                                lmBuffer = lmap.normal;
                            }
                            float3 denoised = UvBilinearClamp_f3(lmBuffer, i2_s(lmap.size), lmUv);
                            diffuseGI = f3_f4(denoised, 1.0f);
                        }

                        float NoR = f1_max(0.0f, f4_dot3(N, R));
                        float4 specularGI = f4_mulvs(diffuseGI, 2.0f * NoR * NoR);

                        float4 indirect = IndirectBRDF(V, N, diffuseGI, specularGI, albedo, rome.x, rome.z, rome.y);

                        lighting = f4_add(lighting, indirect);
                        if (dbgdiffGI)
                        {
                            lighting = diffuseGI;
                        }
                        else if (dbgspecGI)
                        {
                            lighting = specularGI;
                        }
                        else if (dbgUv)
                        {
                            lighting = f4_v(U.x, U.y, 0.0f, 0.0f);
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
