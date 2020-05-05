#include "rendering/fragment_stage.h"
#include "threading/task.h"
#include "common/profiler.h"
#include "allocator/allocator.h"

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

pim_inline float4 VEC_CALL TriBounds(float4x4 VP, float3 A, float3 B, float3 C, i32 iTile)
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

    int2 tile = GetTile(iTile);
    float2 tileMin = TileMin(tile);
    float2 tileMax = TileMax(tile);

    bounds.x = f1_max(bounds.x, tileMin.x);
    bounds.y = f1_max(bounds.y, tileMin.y);
    bounds.z = f1_min(bounds.z, tileMax.x);
    bounds.w = f1_min(bounds.w, tileMax.y);

    return bounds;
}

typedef struct drawmesh_s
{
    task_t task;
    material_t material;
    meshid_t mesh;
    framebuf_t* target;
} drawmesh_t;

static float3 ms_lightDir;
static float3 ms_lightRad;
static float3 ms_diffuseGI;
static float3 ms_specularGI;

float3* LightDir(void) { return &ms_lightDir; }
float3* LightRad(void) { return &ms_lightRad; }
float3* DiffuseGI(void) { return &ms_diffuseGI; }
float3* SpecularGI(void) { return &ms_specularGI; }

ProfileMark(pm_DrawMesh, DrawMesh)
pim_optimize
static void VEC_CALL DrawMesh(drawmesh_t* draw, i32 iTile)
{
    ProfileBegin(pm_DrawMesh);

    mesh_t mesh;
    texture_t albedoMap;
    // texture_t romeMap;
    if (!mesh_get(draw->mesh, &mesh) || !texture_get(draw->material.albedo, &albedoMap))
    {
        return;
    }

    framebuf_t target = *(draw->target);
    const float4* pim_noalias positions = mesh.positions;
    const float4* pim_noalias normals = mesh.normals;
    const float2* pim_noalias uvs = mesh.uvs;
    const i32 vertCount = mesh.length;

    camera_t camera;
    camera_get(&camera);

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

    const float3 tileNormal = proj_dir(right, up, fwd, slope, TileCenter(tile));
    const frus_t frus = frus_new(eye, right, up, fwd, TileMin(tile), TileMax(tile), slope, camera.nearFar);

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

    AcquireTile(&target, iTile);
    for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
    {
        const float3 A = f4_f3(positions[iVert + 0]);
        const float3 B = f4_f3(positions[iVert + 1]);
        const float3 C = f4_f3(positions[iVert + 2]);

        // backface culling
        if (f3_dot(tileNormal, f3_cross(f3_sub(C, A), f3_sub(B, A))) < 0.0f)
        {
            continue;
        }

        // tile-frustum-triangle culling
        if (sdFrusSph(frus, triToSphere(A, B, C)) > 0.0f)
        {
            continue;
        }

        const float3 NA = f4_f3(normals[iVert + 0]);
        const float3 NB = f4_f3(normals[iVert + 1]);
        const float3 NC = f4_f3(normals[iVert + 2]);

        const float2 UA = uvs[iVert + 0];
        const float2 UB = uvs[iVert + 1];
        const float2 UC = uvs[iVert + 2];

        const float4 bounds = TriBounds(VP, A, B, C, iTile);

        const float3 BA = f3_sub(B, A);
        const float3 CA = f3_sub(C, A);
        const float3 T = f3_sub(eye, A);
        const float3 Q = f3_cross(T, BA);
        const float t0 = f3_dot(CA, Q);

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
                    if (t < nearClip || t > farClip || t > target.depth[iTexel])
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
                    target.depth[iTexel] = t;
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
                    const float3 indirect = IndirectBRDF(V, N, diffuseGI, specularGI, albedo, flatRome.x, flatRome.z, flatRome.y);
                    light = f3_add(direct, indirect);
                }

                target.light[iTexel] = f3_f4(light, 1.0f);
            }
        }
    }
    ReleaseTile(&target, iTile);

    ProfileEnd(pm_DrawMesh);
}

ProfileMark(pm_FragmentStageFn, FragmentStageFn)
static void FragmentStageFn(task_t* task, i32 begin, i32 end)
{
    ProfileBegin(pm_FragmentStageFn);

    drawmesh_t* draw = (drawmesh_t*)task;
    for (i32 i = begin; i < end; ++i)
    {
        DrawMesh(draw, i);
    }

    ProfileEnd(pm_FragmentStageFn);
}

ProfileMark(pm_FragmentStage, FragmentStage)
struct task_s* FragmentStage(struct framebuf_s* target, meshid_t worldMesh, material_t material)
{
    ProfileBegin(pm_FragmentStage);

    ASSERT(target);

    drawmesh_t* draw = tmp_calloc(sizeof(*draw));
    draw->target = target;
    draw->mesh = worldMesh;
    draw->material = material;
    task_submit((task_t*)draw, FragmentStageFn, kTileCount);

    ProfileEnd(pm_FragmentStage);
    return (task_t*)draw;
}
