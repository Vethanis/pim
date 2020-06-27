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

typedef enum
{
    DrawMode_Lit = 0,
    DrawMode_Albedo,
    DrawMode_Normal,
    DrawMode_Roughness,
    DrawMode_Occlusion,
    DrawMode_Metallic,
    DrawMode_Emission,
    DrawMode_Uv0,
    DrawMode_Uv1,
    DrawMode_GiDiffuse,
    DrawMode_GiSpecular,
    DrawMode_GiPosition,
    DrawMode_GiNormal,

    DrawMode_COUNT
} DrawMode;

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
    DrawMode mode;
    pt_light_t* lights;
    i32 lightCount;
} tile_ctx_t;

static cvar_t cv_r_albedo = { cvart_bool, 0, "r_albedo", "0", "view albedo" };
static cvar_t cv_r_normal = { cvart_bool, 0, "r_normal", "0", "view normals" };
static cvar_t cv_r_roughness = { cvart_bool, 0, "r_roughness", "0", "view roughness" };
static cvar_t cv_r_occlusion = { cvart_bool, 0, "r_occlusion", "0", "view occlusion" };
static cvar_t cv_r_metallic = { cvart_bool, 0, "r_metallic", "0", "view metallic" };
static cvar_t cv_r_emission = { cvart_bool, 0, "r_emission", "0", "view emission" };
static cvar_t cv_r_uv0 = { cvart_bool, 0, "r_uv0", "0", "view uv0" };
static cvar_t cv_r_uv1 = { cvart_bool, 0, "r_uv1", "0", "view uv1" };
static cvar_t cv_r_gi_diff = { cvart_bool, 0, "r_gi_diff", "0", "view GI diffuse" };
static cvar_t cv_r_gi_spec = { cvart_bool, 0, "r_gi_spec", "0", "view GI specular" };
static cvar_t cv_r_gi_pos = { cvart_bool, 0, "r_gi_pos", "0", "view GI position" };
static cvar_t cv_r_gi_norm = { cvart_bool, 0, "r_gi_norm", "0", "view GI normal" };

static cvar_t cv_r_lm_denoised = { cvart_bool, 0, "r_lm_denoised", "0", "use denoised lightmaps" };

static bool ms_once;
static void EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;
        cvar_reg(&cv_r_albedo);
        cvar_reg(&cv_r_normal);
        cvar_reg(&cv_r_roughness);
        cvar_reg(&cv_r_occlusion);
        cvar_reg(&cv_r_metallic);
        cvar_reg(&cv_r_emission);
        cvar_reg(&cv_r_uv0);
        cvar_reg(&cv_r_uv1);
        cvar_reg(&cv_r_gi_diff);
        cvar_reg(&cv_r_gi_spec);
        cvar_reg(&cv_r_gi_pos);
        cvar_reg(&cv_r_gi_norm);

        cvar_reg(&cv_r_lm_denoised);
    }
}

static DrawMode GetDrawMode(void)
{
    DrawMode mode = DrawMode_Lit;
    if (cvar_get_bool(&cv_r_albedo))
    {
        mode = DrawMode_Albedo;
    }
    else if (cvar_get_bool(&cv_r_normal))
    {
        mode = DrawMode_Normal;
    }
    else if (cvar_get_bool(&cv_r_roughness))
    {
        mode = DrawMode_Roughness;
    }
    else if (cvar_get_bool(&cv_r_occlusion))
    {
        mode = DrawMode_Occlusion;
    }
    else if (cvar_get_bool(&cv_r_metallic))
    {
        mode = DrawMode_Metallic;
    }
    else if (cvar_get_bool(&cv_r_emission))
    {
        mode = DrawMode_Emission;
    }
    else if (cvar_get_bool(&cv_r_uv0))
    {
        mode = DrawMode_Uv0;
    }
    else if (cvar_get_bool(&cv_r_uv1))
    {
        mode = DrawMode_Uv1;
    }
    else if (cvar_get_bool(&cv_r_gi_diff))
    {
        mode = DrawMode_GiDiffuse;
    }
    else if (cvar_get_bool(&cv_r_gi_spec))
    {
        mode = DrawMode_GiSpecular;
    }
    else if (cvar_get_bool(&cv_r_gi_pos))
    {
        mode = DrawMode_GiPosition;
    }
    else if (cvar_get_bool(&cv_r_gi_norm))
    {
        mode = DrawMode_GiNormal;
    }
    return mode;
}

static void SetupTile(tile_ctx_t* ctx, i32 iTile);
static void VEC_CALL DrawMesh(const tile_ctx_t* ctx, framebuf_t* target, i32 iDraw, const Cubemap* cm);
static void VEC_CALL DrawLights(const tile_ctx_t* ctx, framebuf_t* target);
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
        SetupTile(&ctx, iTile);

        u64 tilemask = 1;
        tilemask <<= iTile;

        for (i32 iDraw = 0; iDraw < drawCount; ++iDraw)
        {
            if (tileMasks[iDraw] & tilemask)
            {
                DrawMesh(&ctx, stage->frontBuf, iDraw, NULL);
            }
        }

        DrawLights(&ctx, stage->frontBuf);
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
    A.z = f1_max(-A.z, kEpsilon);
    B.z = f1_max(-B.z, kEpsilon);
    C.z = f1_max(-C.z, kEpsilon);
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
static void SetupTile(tile_ctx_t* ctx, i32 iTile)
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

    ctx->mode = GetDrawMode();

    i32 lightCount = 0;
    pt_light_t* lights = NULL;
    const i32 ptCount = lights_pt_count();
    for (i32 i = 0; i < ptCount; ++i)
    {
        pt_light_t light = lights_get_pt(i);
        sphere_t sph = { light.pos };
        sph.value.w = SphereAttenRadius(light.rad, light.pos.w);
        if (sdFrusSph(ctx->frus, sph) < 0.0f)
        {
            ++lightCount;
            lights = tmp_realloc(lights, sizeof(lights[0]) * lightCount);
            lights[lightCount - 1] = light;
        }
    }
    ctx->lights = lights;
    ctx->lightCount = lightCount;
}

static void VEC_CALL DrawLights(const tile_ctx_t* ctx, framebuf_t* target)
{
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;

    const pt_light_t* pim_noalias ptLights = ctx->lights;
    const i32 ptCount = ctx->lightCount;

    const float4 eye = ctx->eye;
    const float4 fwd = ctx->fwd;
    const float4 right = ctx->right;
    const float4 up = ctx->up;
    const float2 slope = ctx->slope;
    const float nearClip = ctx->nearClip;
    const float farClip = ctx->farClip;
    const float2 tileMin = ctx->tileMin;
    const float2 tileMax = ctx->tileMax;

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    for (i32 iLight = 0; iLight < ptCount; ++iLight)
    {
        float4 center = ptLights[iLight].pos;
        float4 rad = ptLights[iLight].rad;
        sphere_t sph = { center };
        if (sdFrusSph(ctx->frus, sph) > 0.0f)
        {
            continue;
        }
        for (float y = tileMin.y; y < tileMax.y; y += dy)
        {
            for (float x = tileMin.x; x < tileMax.x; x += dx)
            {
                float2 coord = { x, y };
                float4 rd = proj_dir(right, up, fwd, slope, coord);
                ray_t ray = { eye, rd };
                float t = isectSphere3D(ray, sph);
                if ((t <= nearClip) || (t >= farClip))
                {
                    continue;
                }
                i32 iTexel = SnormToIndex(coord);
                if (t < dstDepth[iTexel])
                {
                    dstDepth[iTexel] = t;
                    float4 hitPt = f4_add(eye, f4_mulvs(rd, t));
                    float4 N = f4_normalize3(f4_sub(hitPt, center));
                    float NoL = f1_abs(f4_dot(N, rd));
                    dstLight[iTexel] = f4_mulvs(rad, NoL);
                }
            }
        }
    }
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
    const i32 lmCount = lmpack->lmCount;
    const lightmap_t* lightmaps = lmpack->lightmaps;

    mesh_t mesh;
    if (!mesh_get(drawTable->meshes[iDraw], &mesh))
    {
        return;
    }

    const DrawMode mode = ctx->mode;
    const bool denoisedLM = cvar_get_bool(&cv_r_lm_denoised);

    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;

    const float4x4 M = drawTable->matrices[iDraw];
    const float3x3 IM = f3x3_IM(M);
    const material_t material = drawTable->materials[iDraw];
    const lm_uvs_t lmUvs = drawTable->lmUvs[iDraw];
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
    const i32 vertCount = mesh.length;

    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float4 A = f4x4_mul_pt(M, positions[iVert + 0]);
        const float4 B = f4x4_mul_pt(M, positions[iVert + 1]);
        const float4 C = f4x4_mul_pt(M, positions[iVert + 2]);

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

        const float2 UA = TransformUv(uvs[iVert + 0], material.st);
        const float2 UB = TransformUv(uvs[iVert + 1], material.st);
        const float2 UC = TransformUv(uvs[iVert + 2], material.st);

        const float4 NA = f3x3_mul_col(IM, normals[iVert + 0]);
        const float4 NB = f3x3_mul_col(IM, normals[iVert + 1]);
        const float4 NC = f3x3_mul_col(IM, normals[iVert + 2]);

        const i32 iFace = iVert / 3;
        const i32 lmIndex = (iFace < lmUvs.length) ? lmUvs.indices[iFace] : -1;
        const bool hasLM = (lmIndex >= 0) && (lmIndex < lmCount);
        const float2 LMA = hasLM ? lmUvs.uvs[iVert + 0] : f2_0;
        const float2 LMB = hasLM ? lmUvs.uvs[iVert + 1] : f2_0;
        const float2 LMC = hasLM ? lmUvs.uvs[iVert + 2] : f2_0;

        const float4 bounds = TriBounds(ctx->V, slope, nearClip, farClip, A, B, C, tileMin, tileMax);
        for (float y = bounds.y; y < bounds.w; y += dy)
        {
            for (float x = bounds.x; x < bounds.z; x += dx)
            {
                const float2 coord = { x, y };
                const float4 rd = proj_dir(right, up, fwd, slope, coord);
                const float4 rdXca = f4_cross3(rd, CA);
                const float det = f4_dot3(BA, rdXca);
                if (det < kEpsilon)
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
                    float4 P = f4_blend(A, B, C, wuvt);
                    float4 N = f4_normalize3(f4_blend(NA, NB, NC, wuvt));
                    float2 U = f2_frac(f2_blend(UA, UB, UC, wuvt));
                    float2 LM = f2_blend(LMA, LMB, LMC, wuvt);

                    float4 V = f4_normalize3(f4_sub(eye, P));

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

                    float roughness = rome.x;
                    float occlusion = rome.y;
                    float metallic = rome.z;
                    float4 emission = UnpackEmission(albedo, rome.w);

                    float4 diffuseGI = f4_0;
                    if (hasLM)
                    {
                        const lightmap_t lmap = lightmaps[lmIndex];
                        diffuseGI = f3_f4(UvBilinearClamp_f3(denoisedLM ? lmap.denoised : lmap.color, i2_s(lmap.size), LM), 0.0f);
                    }

                    float4 specularGI = f4_0;
                    {
                        float4 R = f4_reflect3(f4_neg(V), N);
                        float alpha = BrdfAlpha(roughness);
                        R = SpecularDominantDir(N, R, alpha);
                        float NoR = f1_saturate(f4_dot3(N, R));
                        specularGI = f4_mulvs(diffuseGI, NoR);
                    }

                    float4 indirectLight = IndirectBRDF(
                        V,
                        N,
                        diffuseGI,
                        specularGI,
                        albedo,
                        roughness,
                        metallic,
                        occlusion);

                    float4 directLight = f4_0;
                    {
                        const pt_light_t* pim_noalias lights = ctx->lights;
                        const i32 lightCount = ctx->lightCount;
                        for (i32 i = 0; i < lightCount; ++i)
                        {
                            float4 lightPos = lights[i].pos;
                            float4 lightRad = lights[i].rad;
                            float4 Li = EvalSphereLight(
                                V,
                                P,
                                N,
                                albedo,
                                roughness,
                                metallic,
                                lightPos,
                                lightRad,
                                lightPos.w);
                            directLight = f4_add(directLight, Li);
                        }
                    }

                    switch (mode)
                    {
                        default:
                        case DrawMode_Lit:
                            lighting = f4_add(emission, f4_add(directLight, indirectLight));
                        break;
                        case DrawMode_Albedo:
                            lighting = albedo;
                        break;
                        case DrawMode_Normal:
                            lighting = f4_unorm(N);
                        break;
                        case DrawMode_Roughness:
                            lighting = f4_s(rome.x);
                        break;
                        case DrawMode_Occlusion:
                            lighting = f4_s(rome.y);
                        break;
                        case DrawMode_Metallic:
                            lighting = f4_s(rome.z);
                        break;
                        case DrawMode_Emission:
                            lighting = f4_s(rome.w);
                        break;
                        case DrawMode_Uv0:
                            lighting = f4_v(U.x, U.y, 0.0f, 0.0f);
                        break;
                        case DrawMode_Uv1:
                            lighting = f4_v(LM.x, LM.y, 0.0f, 0.0f);
                        break;
                        case DrawMode_GiDiffuse:
                            lighting = diffuseGI;
                        break;
                        case DrawMode_GiSpecular:
                            lighting = specularGI;
                        break;
                        case DrawMode_GiPosition:
                        {
                            if (hasLM)
                            {
                                const lightmap_t lmap = lightmaps[lmIndex];
                                lighting = f3_f4(UvBilinearClamp_f3(lmap.position, i2_s(lmap.size), LM), 0.0f);
                                lighting = f4_divvs(f4_addvs(lighting, 20.0f), 40.0f);
                            }
                        }
                        break;
                        case DrawMode_GiNormal:
                        {
                            if (hasLM)
                            {
                                const lightmap_t lmap = lightmaps[lmIndex];
                                lighting = f3_f4(UvBilinearClamp_f3(lmap.normal, i2_s(lmap.size), LM), 0.0f);
                                lighting = f4_unorm(lighting);
                            }
                        }
                        break;
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
