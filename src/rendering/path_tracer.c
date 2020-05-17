#include "rendering/path_tracer.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/sampler.h"
#include "rendering/lights.h"

#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/int2_funcs.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sampling.h"
#include "math/sdf.h"
#include "math/frustum.h"
#include "math/color.h"

#include "allocator/allocator.h"
#include "components/ecs.h"
#include "threading/task.h"
#include "common/random.h"

typedef enum
{
    hit_nothing = 0,
    hit_triangle,
    hit_ptlight,

    hit_COUNT
} hittype_t;

typedef struct surfhit_s
{
    float4 P;
    float4 N;
    float4 albedo;
    float roughness;
    float occlusion;
    float metallic;
    float4 emission;
} surfhit_t;

typedef struct rayhit_s
{
    float4 wuvt;
    hittype_t type;
    i32 index;
} rayhit_t;

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

static i32 VEC_CALL CalcNodeCount(i32 maxDepth)
{
    i32 nodeCount = 0;
    for (i32 i = 0; i < maxDepth; ++i)
    {
        nodeCount += 1 << (3 * i);
    }
    return nodeCount;
}

static prng_t ms_rngs[kNumThreads];

pim_inline i32 VEC_CALL GetChild(i32 parent, i32 i)
{
    return (parent << 3) | (i + 1);
}

pim_inline i32 VEC_CALL GetParent(i32 i)
{
    return (i - 1) >> 3;
}

static void SetupBounds(box_t* boxes, i32 p, i32 nodeCount)
{
    const i32 c0 = GetChild(p, 0);
    if ((c0 + 8) <= nodeCount)
    {
        {
            const float4 pcenter = boxes[p].center;
            const float4 extents = f4_mulvs(boxes[p].extents, 0.5f);
            for (i32 i = 0; i < 8; ++i)
            {
                float4 sign;
                sign.x = (i & 1) ? -1.0f : 1.0f;
                sign.y = (i & 2) ? -1.0f : 1.0f;
                sign.z = (i & 4) ? -1.0f : 1.0f;
                boxes[c0 + i].center = f4_add(pcenter, f4_mul(sign, extents));
                boxes[c0 + i].extents = extents;
            }
        }
        for (i32 i = 0; i < 8; ++i)
        {
            SetupBounds(boxes, c0 + i, nodeCount);
        }
    }
}

// tests whether the given box fully contains the entire triangle
static bool VEC_CALL BoxHoldsTri(box_t box, float4 A, float4 B, float4 C)
{
    float4 blo = f4_sub(box.center, box.extents);
    float4 bhi = f4_add(box.center, box.extents);
    blo.w = 0.0f;
    bhi.w = 0.0f;
    A.w = 0.0f;
    B.w = 0.0f;
    C.w = 0.0f;
    bool4 lo = b4_and(b4_and(f4_gteq(A, blo), f4_gteq(B, blo)), f4_gteq(C, blo));
    bool4 hi = b4_and(b4_and(f4_lteq(A, bhi), f4_lteq(B, bhi)), f4_lteq(C, bhi));
    return b4_all(b4_and(lo, hi));
}

// tests whether the given box fully contains the entire sphere
static bool VEC_CALL BoxHoldsSph(box_t box, float4 sph)
{
    float4 blo = f4_sub(box.center, box.extents);
    float4 bhi = f4_add(box.center, box.extents);
    float4 slo = f4_subvs(sph, sph.w);
    float4 shi = f4_addvs(sph, sph.w);
    blo.w = 0.0f;
    bhi.w = 0.0f;
    slo.w = 0.0f;
    shi.w = 0.0f;
    return b4_all(b4_and(f4_gteq(slo, blo), f4_lteq(shi, bhi)));
}

static i32 SublistLen(const i32* sublist)
{
    return sublist ? sublist[0] : 0;
}

static i32* SublistPush(i32* sublist, i32 x)
{
    i32 len = SublistLen(sublist);
    ++len;
    sublist = perm_realloc(sublist, sizeof(sublist[0]) * (1 + len));
    sublist[0] = len;
    sublist[len] = x;
    return sublist;
}

static i32 SublistGet(const i32* sublist, i32 i)
{
    ASSERT(i >= 0);
    ASSERT(i < SublistLen(sublist));
    return sublist[1 + i];
}

static bool InsertTriangle(
    pt_scene_t* scene,
    i32 iVert,
    i32 n)
{
    if (n < scene->nodeCount)
    {
        if (BoxHoldsTri(
            scene->boxes[n],
            scene->positions[iVert + 0],
            scene->positions[iVert + 1],
            scene->positions[iVert + 2]))
        {
            scene->pops[n] += 1;
            for (i32 i = 0; i < 8; ++i)
            {
                if (InsertTriangle(
                    scene,
                    iVert,
                    GetChild(n, i)))
                {
                    return true;
                }
            }
            scene->trilists[n] = SublistPush(scene->trilists[n], iVert);
            return true;
        }
    }
    return false;
}

static bool InsertPtLight(
    pt_scene_t* scene,
    i32 iLight,
    i32 n)
{
    if (n < scene->nodeCount)
    {
        if (BoxHoldsSph(
            scene->boxes[n],
            lights_get_pt(iLight).pos))
        {
            scene->pops[n] += 1;
            for (i32 i = 0; i < 8; ++i)
            {
                if (InsertPtLight(
                    scene,
                    iLight,
                    GetChild(n, i)))
                {
                    return true;
                }
            }
            scene->lightlists[n] = SublistPush(scene->lightlists[n], iLight);
            return true;
        }
    }
    return false;
}

pt_scene_t* pt_scene_new(i32 maxDepth)
{
    pt_scene_t* pim_noalias scene = perm_calloc(sizeof(*scene));

    const u32 meshQuery = (1 << CompId_Drawable) | (1 << CompId_LocalToWorld);

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
                float4 position = f4x4_mul_pt(M, mesh.positions[j]);
                positions[vertBack + j] = position;
                sceneMin = f4_min(sceneMin, position);
                sceneMax = f4_max(sceneMax, position);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                normals[vertBack + j] = f4x4_mul_dir(IM, mesh.normals[j]);
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

    {
        const i32 nodeCount = CalcNodeCount(maxDepth);
        scene->nodeCount = nodeCount;
        scene->boxes = perm_calloc(sizeof(scene->boxes[0]) * nodeCount);;
        scene->trilists = perm_calloc(sizeof(scene->trilists[0]) * nodeCount);
        scene->lightlists = perm_calloc(sizeof(scene->lightlists[0]) * nodeCount);
        scene->pops = perm_calloc(sizeof(scene->pops[0]) * nodeCount);

        // scale bounds up a little bit, to account for fp precision
        // abs required when scene is not centered at origin
        sceneMax = f4_add(sceneMax, f4_mulvs(f4_abs(sceneMax), 0.01f));
        sceneMin = f4_sub(sceneMin, f4_mulvs(f4_abs(sceneMin), 0.01f));
        box_t bounds;
        bounds.center = f4_lerpvs(sceneMin, sceneMax, 0.5f);
        bounds.extents = f4_sub(sceneMax, bounds.center);
        scene->boxes[0] = bounds;
        SetupBounds(scene->boxes, 0, nodeCount);

        for (i32 i = 0; (i + 3) <= vertCount; i += 3)
        {
            if (!InsertTriangle(scene, i, 0))
            {
                ASSERT(false);
            }
        }

        const i32 ptLightCount = lights_pt_count();
        for (i32 i = 0; i < ptLightCount; ++i)
        {
            InsertPtLight(scene, i, 0);
        }

    }

    return scene;
}

void pt_scene_del(pt_scene_t* scene)
{
    if (scene)
    {
        pim_free(scene->positions);
        pim_free(scene->normals);
        pim_free(scene->uvs);

        pim_free(scene->materials);

        pim_free(scene->boxes);
        for (i32 i = 0; i < scene->nodeCount; ++i)
        {
            pim_free(scene->trilists[i]);
            pim_free(scene->lightlists[i]);
        }
        pim_free(scene->trilists);
        pim_free(scene->lightlists);
        pim_free(scene->pops);

        memset(scene, 0, sizeof(*scene));
        pim_free(scene);
    }
}

pim_optimize
static rayhit_t VEC_CALL TraceRay(
    const pt_scene_t* scene,
    i32 n,
    ray_t ray,
    float4 rcpRd,
    float limit)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = limit;

    if ((n >= scene->nodeCount) || (scene->pops[n] == 0))
    {
        return hit;
    }

    const float e = 0.001f;

    // test box
    const float2 nearFar = isectBox3D(ray.ro, rcpRd, scene->boxes[n]);
    if ((nearFar.y <= nearFar.x) || // miss
        (nearFar.x > limit) || // further away, culled
        (nearFar.y < e)) // behind eye
    {
        return hit;
    }

    // test triangles
    {
        const i32* list = scene->trilists[n];
        const i32 len = SublistLen(list);
        const float4* pim_noalias vertices = scene->positions;

        for (i32 i = 0; i < len; ++i)
        {
            const i32 j = SublistGet(list, i);

            float4 tri = isectTri3D(
                ray.ro, ray.rd, vertices[j + 0], vertices[j + 1], vertices[j + 2]);
            float t = tri.w;
            if (t < e || t > hit.wuvt.w)
            {
                continue;
            }
            if (b4_any(f4_ltvs(tri, 0.0f)))
            {
                continue;
            }
            hit.type = hit_triangle;
            hit.index = j;
            hit.wuvt = tri;
        }
    }

    // test pt lights
    {
        const i32* list = scene->lightlists[n];
        const i32 len = SublistLen(list);
        const pt_light_t* lights = lights_get()->ptLights;

        for (i32 i = 0; i < len; ++i)
        {
            const i32 j = SublistGet(list, i);
            ASSERT(j < lights_pt_count());
            const pt_light_t light = lights[j];

            float t = isectSphere3D(ray.ro, ray.rd, light.pos, light.pos.w);
            if (t < e || t > hit.wuvt.w)
            {
                continue;
            }
            hit.type = hit_ptlight;
            hit.index = j;
            hit.wuvt.w = t;
        }
    }

    // test children
    for (i32 i = 0; i < 8; ++i)
    {
        rayhit_t child = TraceRay(
            scene, GetChild(n, i), ray, rcpRd, hit.wuvt.w);
        if (child.type != hit_nothing)
        {
            if (child.wuvt.w < hit.wuvt.w)
            {
                hit = child;
            }
        }
    }

    return hit;
}

pim_inline float4 VEC_CALL RandomInUnitSphere(prng_t* rng)
{
    return SampleUnitSphere(f2_v(prng_f32(rng), prng_f32(rng)));
}

pim_inline float4 VEC_CALL GetVert4(const float4* vertices, rayhit_t hit)
{
    return f4_blend(
        vertices[hit.index + 0],
        vertices[hit.index + 1],
        vertices[hit.index + 2],
        hit.wuvt);
}

pim_inline float2 VEC_CALL GetVert2(const float2* vertices, rayhit_t hit)
{
    return f2_blend(
        vertices[hit.index + 0],
        vertices[hit.index + 1],
        vertices[hit.index + 2],
        hit.wuvt);
}

pim_inline const material_t* VEC_CALL GetMaterial(const pt_scene_t* scene, rayhit_t hit)
{
    i32 iMat = (i32)(scene->normals[hit.index].w);
    ASSERT(iMat >= 0);
    ASSERT(iMat < scene->matCount);
    return scene->materials + iMat;
}

pim_inline bool VEC_CALL ScatterLambertian(
    prng_t* rng,
    ray_t rin,
    const surfhit_t* surf,
    float4* attenuation,
    ray_t* scattered)
{
    scattered->ro = surf->P;
    scattered->rd = f4_normalize3(f4_add(surf->N, RandomInUnitSphere(rng)));
    *attenuation = surf->albedo;
    return true;
}

pim_inline bool VEC_CALL ScatterMetallic(
    prng_t* rng,
    ray_t rin,
    const surfhit_t* surf,
    float4* attenuation,
    ray_t* scattered)
{
    float4 R = f4_normalize3(f4_reflect3(rin.rd, surf->N));
    float4 dir = f4_add(R, f4_mulvs(RandomInUnitSphere(rng), surf->roughness));
    scattered->ro = surf->P;
    scattered->rd = f4_normalize3(dir);
    *attenuation = surf->albedo;
    return f4_dot3(scattered->rd, surf->N) > 0.0f;
}

pim_inline bool VEC_CALL Scatter(
    prng_t* rng,
    ray_t rin,
    const surfhit_t* surf,
    float4* attenuation,
    ray_t* scattered)
{
    if (surf->metallic < 0.5f)
    {
        return ScatterLambertian(rng, rin, surf, attenuation, scattered);
    }
    return ScatterMetallic(rng, rin, surf, attenuation, scattered);
}

pim_optimize
static surfhit_t VEC_CALL GetSurface(
    const pt_scene_t* scene,
    ray_t rin,
    rayhit_t hit)
{
    surfhit_t surf = { 0 };

    switch (hit.type)
    {
    default: return surf;
    case hit_triangle:
    {
        ASSERT(hit.index < scene->vertCount);
        const material_t* mat = GetMaterial(scene, hit);
        const float2 uv = GetVert2(scene->uvs, hit);
        surf.P = GetVert4(scene->positions, hit);
        surf.N = f4_normalize3(GetVert4(scene->normals, hit));
        surf.albedo = material_albedo(mat, uv);
        float4 rome = material_rome(mat, uv);
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
        surf.emission = UnpackEmission(surf.albedo, rome.w);
    }
    break;
    case hit_ptlight:
    {
        ASSERT(hit.index < lights_pt_count());
        pt_light_t light = lights_get_pt(hit.index);
        surf.emission = light.rad;
        surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
        surf.N = f4_normalize3(f4_sub(surf.P, light.pos));
        surf.roughness = 1.0f;
        surf.occlusion = 1.0f;
        surf.metallic = 0.0f;
        surf.albedo = f4_0;
    }
    break;
    }

    return surf;
}

pim_optimize
float4 VEC_CALL pt_trace_frag(
    struct prng_s* rng,
    const pt_scene_t* scene,
    ray_t ray,
    i32 bounces)
{
    float4 light = f4_0;
    float4 attenuation = f4_1;
    surfhit_t surf;
    rayhit_t hit;
    for (i32 b = 0; b < bounces; ++b)
    {
        hit = TraceRay(scene, 0, ray, f4_rcp(ray.rd), 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }

        surf = GetSurface(scene, ray, hit);
        light = f4_add(light, f4_mul(attenuation, surf.emission));

        ray_t newRay;
        float4 newAtten;
        if (Scatter(rng, ray, &surf, &newAtten, &newRay))
        {
            ray = newRay;
            if (b >= 3)
            {
                const float p = f4_hmax3(newAtten);
                if (prng_f32(rng) < p)
                {
                    newAtten = f4_mulvs(newAtten, 1.0f / p);
                }
                else
                {
                    break;
                }
            }
            attenuation = f4_mul(attenuation, newAtten);
            if (f4_hmax3(attenuation) == 0.0f)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return light;
}

static float2 f2_tent(prng_t* rng)
{
    float2 t = { prng_f32(rng), prng_f32(rng) };
    t = f2_mulvs(t, 2.0f);
    t.x = t.x < 1.0f ? sqrtf(t.x) - 1.0f : 1.0f - sqrtf(2.0f - t.x);
    t.y = t.y < 1.0f ? sqrtf(t.y) - 1.0f : 1.0f - sqrtf(2.0f - t.x);
    return t;
}

pim_optimize
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

    const i32 tid = task_thread_id();
    prng_t rng = ms_rngs[tid];
    if (rng.state == 0)
    {
        rng = prng_create();
    }

    const float2 dSize = f2_rcp(i2_f2(size));
    for (i32 i = begin; i < end; ++i)
    {
        int2 xy = { i % size.x, i / size.x };

        float2 coord = f2_addvs(i2_f2(xy), 0.5f);
        coord = f2_add(coord, f2_tent(&rng));
        coord = f2_snorm(f2_mul(coord, dSize));

        float4 rd = proj_dir(right, up, fwd, slope, coord);
        ray_t ray = { ro, rd };
        float4 sample = pt_trace_frag(&rng, scene, ray, bounces);
        image[i] = f4_lerpvs(image[i], sample, sampleWeight);
    }

    ms_rngs[tid] = rng;
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
