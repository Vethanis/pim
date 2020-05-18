#include "rendering/fragment_stage.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "components/ecs.h"
#include "containers/ptrqueue.h"
#include "threading/task.h"

#include "math/types.h"
#include "math/float4x4_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/lighting.h"
#include "math/color.h"
#include "math/frustum.h"
#include "math/sdf.h"
#include "math/sphgauss.h"

#include "rendering/sampler.h"
#include "rendering/tile.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/camera.h"
#include "rendering/framebuffer.h"
#include "rendering/lights.h"

typedef struct fragstage_s
{
    task_t task;
    framebuf_t* target;
} fragstage_t;

typedef struct drawmesh_s
{
    material_t material;
    meshid_t mesh;
} drawmesh_t;

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
} tile_ctx_t;

static cvar_t cv_sg_dbgirad = { cvart_bool, 0, "sg_dbgirad", "0", "display a debug view of diffuse GI" };
static cvar_t cv_sg_dbgeval = { cvart_bool, 0, "sg_dbgeval", "0", "display a debug view of specular GI" };

static ptrqueue_t ms_fragQueues[kTileCount];
static BrdfLut ms_lut;
static float4 ms_diffuseGI;
static float4 ms_specularGI;
static u64 ms_trisCulled;
static u64 ms_trisDrawn;

static i32 ms_sgcount;
static SG_t ms_sgs[256];

SG_t* SG_Get(void) { return ms_sgs; }
i32 SG_GetCount(void) { return ms_sgcount; }
void SG_SetCount(i32 ct) { ms_sgcount = i1_clamp(ct, 0, NELEM(ms_sgs)); }
float4* DiffuseGI(void) { return &ms_diffuseGI; }
float4* SpecularGI(void) { return &ms_specularGI; }
u64 Frag_TrisCulled(void) { return ms_trisCulled; }
u64 Frag_TrisDrawn(void) { return ms_trisDrawn; }

static void SetupTile(tile_ctx_t* ctx, i32 iTile);
static float4 VEC_CALL TriBounds(float4x4 VP, float4 A, float4 B, float4 C, float2 tileMin, float2 tileMax);
static void VEC_CALL DrawMesh(const tile_ctx_t* ctx, framebuf_t* target, const drawmesh_t* draw);

pim_optimize
static void FragmentStageFn(task_t* task, i32 begin, i32 end)
{
    fragstage_t* stage = (fragstage_t*)task;
    framebuf_t* pim_noalias target = stage->target;
    tile_ctx_t ctx;

    for (i32 i = begin; i < end; ++i)
    {
        SetupTile(&ctx, i);

        ptrqueue_t* queue = ms_fragQueues + i;
        drawmesh_t* draw = NULL;
    trypop:
        draw = ptrqueue_trypop(queue);
        if (draw)
        {
            DrawMesh(&ctx, target, draw);
            goto trypop;
        }
    }
}

void SubmitMesh(meshid_t worldMesh, material_t material)
{
    drawmesh_t* draw = tmp_calloc(sizeof(*draw));
    draw->material = material;
    draw->mesh = worldMesh;

    for (i32 i = 0; i < kTileCount; ++i)
    {
        bool pushed = ptrqueue_trypush(ms_fragQueues + i, draw);
        ASSERT(pushed);
    }
}

void FragmentStage_Init(void)
{
    cvar_reg(&cv_sg_dbgirad);
    cvar_reg(&cv_sg_dbgeval);

    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_create(ms_fragQueues + i, EAlloc_Perm, 8192);
    }
    ms_lut = BakeBRDF(i2_s(256), 1024);
}

void FragmentStage_Shutdown(void)
{
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_destroy(ms_fragQueues + i);
    }
    FreeBrdfLut(&ms_lut);
}

ProfileMark(pm_FragmentStage, FragmentStage)
struct task_s* FragmentStage(struct framebuf_s* target)
{
    ProfileBegin(pm_FragmentStage);

    ASSERT(target);

#if CULLING_STATS
    store_u64(&ms_trisCulled, 0, MO_Release);
    store_u64(&ms_trisDrawn, 0, MO_Release);
#endif // CULLING_STATS

    fragstage_t* stage = tmp_calloc(sizeof(*stage));
    stage->target = target;
    task_submit((task_t*)stage, FragmentStageFn, kTileCount);

    ProfileEnd(pm_FragmentStage);
    return (task_t*)stage;
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
    float4x4 P = f4x4_perspective(f1_radians(camera.fovy), kDrawAspect, ctx->nearClip, ctx->farClip);
    ctx->VP = f4x4_mul(P, V);
}

pim_optimize
static void VEC_CALL DrawMesh(const tile_ctx_t* ctx, framebuf_t* target, const drawmesh_t* draw)
{
    mesh_t mesh;
    texture_t albedoMap;
    texture_t romeMap;
    if (!mesh_get(draw->mesh, &mesh))
    {
        return;
    }

    const bool dbgdiffGI = cv_sg_dbgirad.asFloat != 0.0f;
    const bool dbgspecGI = cv_sg_dbgeval.asFloat != 0.0f;

    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float e = 1.0f / (1 << 10);

    const BrdfLut lut = ms_lut;
    const float4 flatAlbedo = ColorToLinear(draw->material.flatAlbedo);
    const float4 flatRome = ColorToLinear(draw->material.flatRome);
    const bool hasAlbedo = texture_get(draw->material.albedo, &albedoMap);
    const bool hasRome = texture_get(draw->material.rome, &romeMap);

    const float4 eye = ctx->eye;
    const float4 fwd = ctx->fwd;
    const float4 right = ctx->right;
    const float4 up = ctx->up;
    const float2 slope = ctx->slope;
    const float nearClip = ctx->nearClip;
    const float farClip = ctx->farClip;
    const float2 tileMin = ctx->tileMin;
    const float2 tileMax = ctx->tileMax;

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

        // backface culling
        if (f4_dot3(ctx->tileNormal, f4_cross3(CA, BA)) < 0.0f)
        {
            continue;
        }

        // tile-frustum-triangle culling
        const box_t triBox = triToBox(A, B, C);
        const float boxDist = sdFrusBox(ctx->frus, triBox);
        if (boxDist > 0.0f)
        {
#if CULLING_STATS
            inc_u64(&ms_trisCulled, MO_Relaxed);
#endif // CULLING_STATS
            continue;
        }
#if CULLING_STATS
        inc_u64(&ms_trisDrawn, MO_Relaxed);
#endif // CULLING_STATS

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
                    float4 albedo = flatAlbedo;
                    if (hasAlbedo)
                    {
                        albedo = f4_mul(albedo, Tex_Bilinearf2(albedoMap, U));
                    }
                    float4 rome = flatRome;
                    if (hasRome)
                    {
                        rome = f4_mul(rome, Tex_Bilinearf2(romeMap, U));
                    }
                    float4 diffuseGI = f4_0;
                    float4 specularGI = f4_0;
                    float4 R = f4_normalize3(f4_reflect(f4_neg(V), N));
                    {
                        const i32 sgcount = ms_sgcount;
                        const SG_t* pim_noalias sgs = ms_sgs;
                        for (i32 i = 0; i < sgcount; ++i)
                        {
                            float4 diffuse = SG_Irradiance(sgs[i], N);
                            float4 specular = SG_Eval(sgs[i], R);
                            diffuseGI = f4_add(diffuseGI, diffuse);
                            specularGI = f4_add(specularGI, specular);
                        }
                    }
                    for (i32 iLight = 0; iLight < dirCount; ++iLight)
                    {
                        dir_light_t light = dirLights[iLight];
                        const float4 direct = DirectBRDF(V, light.dir, light.rad, N, albedo, rome.x, rome.z);
                        const float4 indirect = IndirectBRDF(lut, V, N, diffuseGI, specularGI, albedo, rome.x, rome.z, rome.y);
                        lighting = f4_add(lighting, f4_add(direct, indirect));
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
                        const float4 indirect = IndirectBRDF(lut, V, N, diffuseGI, specularGI, albedo, rome.x, rome.z, rome.y);
                        lighting = f4_add(lighting, f4_add(direct, indirect));
                    }

                    if (dbgdiffGI)
                    {
                        lighting = f4_0;
                        const i32 sgcount = ms_sgcount;
                        const SG_t* pim_noalias sgs = ms_sgs;
                        for (i32 i = 0; i < sgcount; ++i)
                        {
                            lighting = f4_add(lighting, SG_Irradiance(sgs[i], N));
                        }
                    }
                    else if (dbgspecGI)
                    {
                        lighting = f4_0;
                        const i32 sgcount = ms_sgcount;
                        const SG_t* pim_noalias sgs = ms_sgs;
                        for (i32 i = 0; i < sgcount; ++i)
                        {
                            lighting = f4_add(lighting, SG_Eval(sgs[i], R));
                        }
                    }
                }

                dstLight[iTexel] = lighting;
            }
        }
    }
}
