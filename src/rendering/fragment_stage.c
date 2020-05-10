#include "rendering/fragment_stage.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include "common/profiler.h"
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
    i32 lightCount;
    const float4* lightDirs;
    const float4* lightRads;
} fragstage_t;

typedef struct drawmesh_s
{
    material_t material;
    meshid_t mesh;
} drawmesh_t;

static ptrqueue_t ms_queues[kTileCount];
static BrdfLut ms_lut;
static float4 ms_lightDir;
static float4 ms_lightRad;
static float4 ms_diffuseGI;
static float4 ms_specularGI;
static u64 ms_trisCulled;
static u64 ms_trisDrawn;

float4* LightDir(void) { return &ms_lightDir; }
float4* LightRad(void) { return &ms_lightRad; }
float4* DiffuseGI(void) { return &ms_diffuseGI; }
float4* SpecularGI(void) { return &ms_specularGI; }
u64 Frag_TrisCulled(void) { return ms_trisCulled; }
u64 Frag_TrisDrawn(void) { return ms_trisDrawn; }

static float4 VEC_CALL TriBounds(float4x4 VP, float4 A, float4 B, float4 C, float2 tileMin, float2 tileMax);
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
    ms_lut = BakeBRDF(i2_s(256), 1024);
}

void FragmentStage_Shutdown(void)
{
    for (i32 i = 0; i < kTileCount; ++i)
    {
        ptrqueue_destroy(ms_queues + i);
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

    i32 lightCount = 0;
    const u32 lightQuery = (1 << CompId_Light) | (1 << CompId_Rotation);
    const ent_t* lightEnts = ecs_query(lightQuery, 0, &lightCount);

    light_t* lights = ecs_gather(CompId_Light, lightEnts, lightCount);
    rotation_t* lightRots = ecs_gather(CompId_Rotation, lightEnts, lightCount);

    for (i32 i = 0; i < lightCount; ++i)
    {
        quat rot = lightRots[i].Value;
        lightRots[i].Value.v = quat_fwd(rot);
    }

    fragstage_t* stage = tmp_calloc(sizeof(*stage));
    stage->target = target;
    stage->lightCount = lightCount;
    stage->lightRads = (float4*)lights;
    stage->lightDirs = (float4*)lightRots;
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

    const float4 fwd = quat_fwd(camera.rotation);
    const float4 right = quat_right(camera.rotation);
    const float4 up = quat_up(camera.rotation);
    const float4 eye = camera.position;

    const float2 tileMin = TileMin(tile);
    const float2 tileMax = TileMax(tile);
    const float4 tileNormal = proj_dir(right, up, fwd, slope, f2_lerp(tileMin, tileMax, 0.5f));
    const frus_t frus = frus_new(eye, right, up, fwd, tileMin, tileMax, slope, camera.nearFar);

    float4x4 VP;
    {
        const float4x4 V = f4x4_lookat(eye, f4_add(eye, fwd), up);
        const float4x4 P = f4x4_perspective(f1_radians(camera.fovy), aspect, nearClip, farClip);
        VP = f4x4_mul(P, V);
    }

    const float4 diffuseGI = ms_diffuseGI;
    const float4 specularGI = ms_specularGI;
    const float4 L = f4_normalize3(ms_lightDir);
    const float4 radiance = ms_lightRad;
    const float4 flatAlbedo = ColorToLinear(draw->material.flatAlbedo);
    const float4 flatRome = ColorToLinear(draw->material.flatRome);

    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float4 A = positions[iVert + 0];
        const float4 B = positions[iVert + 1];
        const float4 C = positions[iVert + 2];

        const float4 BA = f4_sub(B, A);
        const float4 CA = f4_sub(C, A);

        // backface culling
        if (f4_dot3(tileNormal, f4_cross3(CA, BA)) < 0.0f)
        {
            continue;
        }

        // tile-frustum-triangle culling
        const box_t triBox = triToBox(A, B, C);
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

        const float4 NA = normals[iVert + 0];
        const float4 NB = normals[iVert + 1];
        const float4 NC = normals[iVert + 2];

        const float2 UA = uvs[iVert + 0];
        const float2 UB = uvs[iVert + 1];
        const float2 UC = uvs[iVert + 2];

        const float4 T = f4_sub(eye, A);
        const float4 Q = f4_cross3(T, BA);
        const float t0 = f4_dot3(CA, Q);

        const float4 bounds = TriBounds(VP, A, B, C, tileMin, tileMax);
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
                    wuvt.w = t0 * rcpDet;
                    if (wuvt.w < nearClip || wuvt.w > farClip || wuvt.w > dstDepth[iTexel])
                    {
                        continue;
                    }
                    wuvt.y = f4_dot3(T, rdXca) * rcpDet;
                    wuvt.z = f4_dot3(rd, Q) * rcpDet;
                    wuvt.x = 1.0f - wuvt.y - wuvt.z;
                    if (wuvt.x < 0.0f || wuvt.y < 0.0f || wuvt.z < 0.0f)
                    {
                        continue;
                    }
                    dstDepth[iTexel] = wuvt.w;
                }

                float4 light;
                {
                    // blend interpolators
                    const float4 P = f4_blend(A, B, C, wuvt);
                    const float4 N = f4_normalize3(f4_blend(NA, NB, NC, wuvt));
                    const float2 U = f2_blend(UA, UB, UC, wuvt);

                    // lighting
                    const float4 V = f4_normalize3(f4_sub(eye, P));
                    const float4 albedo = f4_mul(flatAlbedo, Tex_Bilinearf2(albedoMap, U));
                    const float4 direct = DirectBRDF(V, L, radiance, N, albedo, flatRome.x, flatRome.z);
                    const float4 indirect = IndirectBRDF(lut, V, N, diffuseGI, specularGI, albedo, flatRome.x, flatRome.z, flatRome.y);
                    light = f4_add(direct, indirect);
                    light.w = 1.0f;
                }

                dstLight[iTexel] = light;
            }
        }
    }
}
