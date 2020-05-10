#include "rendering/path_tracer.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/sampler.h"

#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/int2_funcs.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/lighting.h"
#include "math/sampling.h"
#include "math/sdf.h"
#include "math/frustum.h"
#include "math/color.h"

#include "allocator/allocator.h"
#include "components/ecs.h"
#include "threading/task.h"
#include "common/random.h"

pt_scene_t* pt_scene_new(void)
{
    pt_scene_t* pim_noalias scene = perm_calloc(sizeof(*scene));

    const u32 meshQuery =
        (1 << CompId_Drawable) |
        (1 << CompId_LocalToWorld);

    i32 meshCount = 0;
    i32 vertCount = 0;
    i32 matCount = 0;
    ent_t* pim_noalias meshEnts = ecs_query(meshQuery, 0, &meshCount);
    for (i32 i = 0; i < meshCount; ++i)
    {
        const ent_t ent = meshEnts[i];
        const drawable_t* drawable = ecs_get(ent, CompId_Drawable);
        ASSERT(drawable);
        mesh_t mesh;
        if (mesh_get(drawable->mesh, &mesh))
        {
            vertCount += mesh.length;
            ++matCount;
        }
    }

    float4* pim_noalias positions = perm_malloc(sizeof(positions[0]) * vertCount);
    float4* pim_noalias normals = perm_malloc(sizeof(normals[0]) * vertCount);
    float2* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);
    material_t* pim_noalias materials = perm_malloc(sizeof(materials[0]) * matCount);

    const float kBigNum = 1 << 20;
    float4 sceneMin = f4_s(kBigNum);
    float4 sceneMax = f4_s(-kBigNum);
    i32 vertBack = 0;
    i32 matBack = 0;
    for (i32 i = 0; i < meshCount; ++i)
    {
        const ent_t ent = meshEnts[i];
        const drawable_t* drawable = ecs_get(ent, CompId_Drawable);

        mesh_t mesh;
        if (mesh_get(drawable->mesh, &mesh))
        {
            const localtoworld_t* l2w = ecs_get(ent, CompId_LocalToWorld);
            const float4x4 M = l2w->Value;
            const float4x4 IM = f4x4_inverse(f4x4_transpose(M));
            const material_t material = drawable->material;

            for (i32 j = 0; j < mesh.length; ++j)
            {
                positions[vertBack + j] = f4x4_mul_pt(M, mesh.positions[j]);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                const float4 position = positions[vertBack + j];
                sceneMin = f4_min(sceneMin, position);
                sceneMax = f4_max(sceneMax, position);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                normals[vertBack + j] = f4x4_mul_dir(IM, mesh.positions[j]);
                normals[vertBack + j].w = (float)matBack;
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                uvs[vertBack + j] = TransformUv(mesh.uvs[j], material.st);
            }

            vertBack += mesh.length;
            materials[matBack] = material;
            ++matBack;
        }
    }

    ASSERT(vertBack == vertCount);
    ASSERT(matBack == matCount);

    scene->vertCount = vertCount;
    scene->positions = positions;
    scene->normals = normals;
    scene->uvs = uvs;

    scene->matCount = matCount;
    scene->materials = materials;

    sceneMax = f4_addvs(sceneMax, 0.01f);
    sceneMin = f4_addvs(sceneMin, -0.01f);
    box_t bounds;
    bounds.center = f4_lerp(sceneMin, sceneMax, 0.5f);
    bounds.extents = f4_sub(sceneMax, bounds.center);

    return scene;
}

void pt_scene_del(pt_scene_t* scene)
{
    ASSERT(scene);

    pim_free(scene->positions);
    pim_free(scene->normals);
    pim_free(scene->uvs);

    pim_free(scene->materials);

    memset(scene, 0, sizeof(*scene));
    pim_free(scene);
}

typedef struct trace_task_s
{
    task_t task;
    const pt_scene_t* scene;
    camera_t camera;
    float4* image;
    int2 imageSize;
    float sampleWeight;
    i32 bounces;
} trace_task_t;

static i32 VEC_CALL TraceRay(
    i32 vertCount,
    const float4* pim_noalias vertices,
    float4 ro,
    float4 rd,
    float4* wuvtOut)
{
    i32 index = -1;
    float4 wuvt = f4_s(-1.0f);
    float t = 1 << 20;
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        float4 tri = isectTri3D(ro, rd, vertices[i], vertices[i + 1], vertices[i + 2]);
        if (f4_any(f4_ltvs(tri, 0.0f)))
        {
            continue;
        }
        if (tri.w > t)
        {
            continue;
        }
        t = tri.w;
        wuvt = tri;
        index = i;
    }
    *wuvtOut = wuvt;
    return index;
}

pim_inline float4 VEC_CALL RandomRayDir(
    prng_t* pim_noalias rng,
    float4 N)
{
    float4 dir;
    do
    {
        dir = (float4){ prng_f32(rng), prng_f32(rng), prng_f32(rng), 0.0f };
        dir = f4_snorm(dir);
        if (f4_lengthsq3(dir) >= 1.0f)
        {
            continue;
        }
    } while (f4_dot3(N, dir) <= 0.0f);
    return f4_normalize3(dir);
}

static float4 VEC_CALL GetVert4(
    const float4* pim_noalias vertices,
    i32 iVert,
    float4 wuvt)
{
    return f4_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuvt);
}

static float2 VEC_CALL GetVert2(
    const float2* pim_noalias vertices,
    i32 iVert,
    float4 wuvt)
{
    return f2_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuvt);
}

static const material_t* VEC_CALL GetMaterial(
    const pt_scene_t* scene, i32 iVert)
{
    i32 iMat = (i32)(scene->normals[iVert].w);
    ASSERT(iMat >= 0);
    ASSERT(iMat < scene->matCount);
    return scene->materials + iMat;
}

static float4 VEC_CALL GetAlbedo(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatAlbedo);
    texture_t tex;
    if (texture_get(mat->albedo, &tex))
    {
        value = f4_mul(value, Tex_Bilinearf2(tex, uv));
    }
    return value;
}

static float4 VEC_CALL GetRome(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatRome);
    texture_t tex;
    if (texture_get(mat->rome, &tex))
    {
        value = f4_mul(value, Tex_Bilinearf2(tex, uv));
    }
    return value;
}

static float4 VEC_CALL TracePixel(
    prng_t* rng,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    i32 b,
    i32 bounces)
{
    if (b > bounces)
    {
        return f4_s(0.0f);
    }

    float4 wuvt;
    const i32 iVert = TraceRay(
        scene->vertCount,
        scene->positions,
        ro,
        rd,
        &wuvt);
    if (iVert == -1)
    {
        return f4_s(0.0f);
    }

    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    const float4 P = GetVert4(scene->positions, iVert, wuvt);
    const float4 N = f4_normalize3(GetVert4(scene->normals, iVert, wuvt));
    const float2 uv = GetVert2(scene->uvs, iVert, wuvt);

    const material_t* mat = GetMaterial(scene, iVert);
    const float4 albedo = GetAlbedo(mat, uv);
    const float4 rome = GetRome(mat, uv);
    const float4 emission = UnpackEmission(albedo, rome.w);

    const float4 ro2 = f4_add(P, f4_mulvs(N, 0.001f));

    const float4 lPt = f4_v(0.0f, 10.0f, 0.5f, 0.0f);
    float4 L = f4_sub(lPt, P);
    float lDist = f4_length(L);
    L = f4_divvs(L, lDist);
    float cosTheta = f1_saturate(f4_dot3(L, N));
    float4 reflected = f4_s(3.0f * cosTheta / (lDist*lDist));

    const float4 rd2 = RandomRayDir(rng, N);
    cosTheta = f4_dot3(N, rd2);
    if (cosTheta > 0.0f)
    {
        float4 incoming = TracePixel(rng, scene, ro2, rd2, b + 1, bounces);
        f4_mulvs(incoming, cosTheta);
        f4_add(reflected, incoming);
    }

    return f4_add(emission, f4_mul(reflected, albedo));
}

static void TraceFn(task_t* task, i32 begin, i32 end)
{
    trace_task_t* trace = (trace_task_t*)task;

    const pt_scene_t* scene = trace->scene;
    float4* pim_noalias image = trace->image;
    const int2 size = trace->imageSize;
    const float sampleWeight = trace->sampleWeight;
    const i32 bounces = trace->bounces;
    const camera_t camera = trace->camera;
    const quat rot = camera.rotation;
    const float4 ro = camera.position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);

    prng_t rng = prng_create();

    const float2 dSize = f2_rcp(i2_f2(size));
    for (i32 i = begin; i < end; ++i)
    {
        const int2 xy = { i % size.x, i / size.x };
        const float2 jitter = { prng_f32(&rng), prng_f32(&rng) };
        const float2 coord = f2_snorm(f2_mul(f2_add(i2_f2(xy), jitter), dSize));
        const float4 rd = proj_dir(right, up, fwd, slope, coord);
        float4 sample = TracePixel(&rng, scene, ro, rd, 0, bounces);
        image[i] = f4_lerp(image[i], sample, sampleWeight);
    }
}

struct task_s* pt_trace(pt_trace_t* desc)
{
    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(desc->dstImage);
    ASSERT(desc->camera);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->scene = desc->scene;
    task->camera = desc->camera[0];
    task->image = desc->dstImage;
    task->imageSize = desc->imageSize;
    task->sampleWeight = desc->sampleWeight;
    task->bounces = desc->bounces;

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_submit((task_t*)task, TraceFn, workSize);
    return (task_t*)task;
}
