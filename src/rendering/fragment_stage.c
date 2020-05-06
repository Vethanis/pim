#include "rendering/fragment_stage.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
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

#include "rendering/sampler.h"
#include "rendering/tile.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/camera.h"
#include "rendering/framebuffer.h"

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

static ptrqueue_t ms_queues[kTileCount];
static BrdfLut ms_lut;
static float3 ms_lightDir;
static float3 ms_lightRad;
static float3 ms_diffuseGI;
static float3 ms_specularGI;
static u64 ms_trisCulled;
static u64 ms_trisDrawn;

float3* LightDir(void) { return &ms_lightDir; }
float3* LightRad(void) { return &ms_lightRad; }
float3* DiffuseGI(void) { return &ms_diffuseGI; }
float3* SpecularGI(void) { return &ms_specularGI; }
u64 Frag_TrisCulled(void) { return ms_trisCulled; }
u64 Frag_TrisDrawn(void) { return ms_trisDrawn; }

static float4 VEC_CALL TriBounds(float4x4 VP, float3 A, float3 B, float3 C, float2 tileMin, float2 tileMax);
static void VEC_CALL DrawMesh(framebuf_t* target, drawmesh_t* draw, i32 iTile);

pim_optimize
static void FragmentStageFn(task_t* task, i32 begin, i32 end)
{
    fragstage_t* stage = (fragstage_t*)task;
    framebuf_t* pim_noalias target = stage->target;

    for (i32 i = begin; i < end; ++i)
    {
        ptrqueue_t* queue = ms_queues + i;
        drawmesh_t* draw = NULL;
    trypop:
        draw = ptrqueue_trypop(queue);
        if (draw)
        {
            DrawMesh(target, draw, i);
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
        bool pushed = ptrqueue_trypush(ms_queues + i, draw);
        ASSERT(pushed);
    }
}

void FragmentStage_Init(void)
{
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_create(ms_queues + i, EAlloc_Perm, 256);
    }
    ms_lut = BakeBRDF(256, 256, 1024);
}

void FragmentStage_Shutdown(void)
{
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_destroy(ms_queues + i);
    }
    FreeBRDF(&ms_lut);
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
static float4 VEC_CALL TriBounds(float4x4 VP, float3 A, float3 B, float3 C, float2 tileMin, float2 tileMax)
{
    float4 a = f3_f4(A, 1.0f);
    float4 b = f3_f4(B, 1.0f);
    float4 c = f3_f4(C, 1.0f);

    a = f4x4_mul_pt(VP, a);
    b = f4x4_mul_pt(VP, b);
    c = f4x4_mul_pt(VP, c);

    a = f4_divvs(a, a.w);
    b = f4_divvs(b, b.w);
    c = f4_divvs(c, c.w);

    float4 bounds;
    bounds.x = f1_min(a.x, f1_min(b.x, c.x));
    bounds.y = f1_min(a.y, f1_min(b.y, c.y));
    bounds.z = f1_max(a.x, f1_max(b.x, c.x));
    bounds.w = f1_max(a.y, f1_max(b.y, c.y));

    bounds.x = f1_max(bounds.x, tileMin.x);
    bounds.y = f1_max(bounds.y, tileMin.y);
    bounds.z = f1_min(bounds.z, tileMax.x);
    bounds.w = f1_min(bounds.w, tileMax.y);

    return bounds;
}

pim_optimize
static void VEC_CALL DrawMesh(framebuf_t* target, drawmesh_t* draw, i32 iTile)
{
    mesh_t mesh;
    texture_t albedoMap;
    // texture_t romeMap;
    if (!mesh_get(draw->mesh, &mesh) || !texture_get(draw->material.albedo, &albedoMap))
    {
        return;
    }

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    const float4* pim_noalias positions = mesh.positions;
    const float4* pim_noalias normals = mesh.normals;
    const float2* pim_noalias uvs = mesh.uvs;
    const i32 vertCount = mesh.length;

    camera_t camera;
    camera_get(&camera);

    const BrdfLut lut = ms_lut;

    const int2 tile = GetTile(iTile);
    const float aspect = (float)kDrawWidth / kDrawHeight;
    const float dx = 1.0f / kDrawWidth;
    const float dy = 1.0f / kDrawHeight;
    const float e = 1.0f / (1 << 10);
    const float nearClip = camera.nearFar.x;
    const float farClip = camera.nearFar.y;
    const float2 slope = proj_slope(f1_radians(camera.fovy), aspect);

    const float3 fwd = quat_fwd(camera.rotation);
    const float3 right = quat_right(camera.rotation);
    const float3 up = quat_up(camera.rotation);
    const float3 eye = camera.position;

    const float2 tileMin = TileMin(tile);
    const float2 tileMax = TileMax(tile);
    const float3 tileNormal = proj_dir(right, up, fwd, slope, f2_lerp(tileMin, tileMax, 0.5f));
    const frus_t frus = frus_new(eye, right, up, fwd, tileMin, tileMax, slope, camera.nearFar);

    float4x4 VP;
    {
        const float4x4 V = f4x4_lookat(eye, f3_add(eye, fwd), up);
        const float4x4 P = f4x4_perspective(f1_radians(camera.fovy), aspect, nearClip, farClip);
        VP = f4x4_mul(P, V);
    }

    const float3 diffuseGI = ms_diffuseGI;
    const float3 specularGI = ms_specularGI;
    const float3 L = f3_normalize(ms_lightDir);
    const float3 radiance = ms_lightRad;
    const float4 flatAlbedo = rgba8_f4(draw->material.flatAlbedo);
    const float4 flatRome = rgba8_f4(draw->material.flatRome);

    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float3 A = f4_f3(positions[iVert + 0]);
        const float3 B = f4_f3(positions[iVert + 1]);
        const float3 C = f4_f3(positions[iVert + 2]);

        const float3 BA = f3_sub(B, A);
        const float3 CA = f3_sub(C, A);

        // backface culling
        if (f3_dot(tileNormal, f3_cross(CA, BA)) < 0.0f)
        {
            continue;
        }

        // tile-frustum-triangle culling
        const box_t triBox = triToBox(f3_f4(A, 1.0f), f3_f4(B, 1.0f), f3_f4(C, 1.0f));
        const float boxDist = sdFrusBoxTest(frus, triBox);
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

        const float3 NA = f4_f3(normals[iVert + 0]);
        const float3 NB = f4_f3(normals[iVert + 1]);
        const float3 NC = f4_f3(normals[iVert + 2]);

        const float2 UA = uvs[iVert + 0];
        const float2 UB = uvs[iVert + 1];
        const float2 UC = uvs[iVert + 2];

        const float3 T = f3_sub(eye, A);
        const float3 Q = f3_cross(T, BA);
        const float t0 = f3_dot(CA, Q);

        const float4 bounds = TriBounds(VP, A, B, C, tileMin, tileMax);
        for (float y = bounds.y; y < bounds.w; y += dy)
        {
            for (float x = bounds.x; x < bounds.z; x += dx)
            {
                const float2 coord = { x, y };
                const float3 rd = proj_dir(right, up, fwd, slope, coord);
                const float3 rdXca = f3_cross(rd, CA);
                const float det = f3_dot(BA, rdXca);
                if (det < e)
                {
                    continue;
                }

                const i32 iTexel = SnormToIndex(coord);
                float3 wuv;
                {
                    // barycentric and depth clipping
                    const float rcpDet = 1.0f / det;
                    const float t = t0 * rcpDet;
                    if (t < nearClip || t > farClip || t > dstDepth[iTexel])
                    {
                        continue;
                    }
                    const float u = f3_dot(T, rdXca) * rcpDet;
                    if (u < 0.0f)
                    {
                        continue;
                    }
                    const float v = f3_dot(rd, Q) * rcpDet;
                    if (v < 0.0f)
                    {
                        continue;
                    }
                    const float w = 1.0f - u - v;
                    if (w < 0.0f)
                    {
                        continue;
                    }
                    wuv = (float3) { w, u, v };
                    dstDepth[iTexel] = t;
                }

                float3 light;
                {
                    // blend interpolators
                    const float3 P = f3_blend(A, B, C, wuv);
                    const float3 N = f3_normalize(f3_blend(NA, NB, NC, wuv));
                    const float2 U = f2_blend(UA, UB, UC, wuv);

                    // lighting
                    const float3 V = f3_normalize(f3_sub(eye, P));
                    const float3 albedo = f4_f3(f4_mul(Tex_Bilinearf2(albedoMap, U), flatAlbedo));
                    const float3 direct = DirectBRDF(V, L, radiance, N, albedo, flatRome.x, flatRome.z);
                    const float3 indirect = IndirectBRDF(lut, V, N, diffuseGI, specularGI, albedo, flatRome.x, flatRome.z, flatRome.y);
                    light = f3_add(direct, indirect);
                }

                dstLight[iTexel] = f3_f4(light, 1.0f);
            }
        }
    }
}
