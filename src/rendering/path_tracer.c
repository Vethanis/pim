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
#include "math/lighting.h"

#include "allocator/allocator.h"
#include "components/table.h"
#include "components/components.h"
#include "components/drawables.h"
#include "threading/task.h"
#include "common/random.h"

#include <string.h>

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
    trace_img_t img;
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

void trace_img_new(trace_img_t* img, int2 size)
{
    ASSERT(img);
    ASSERT(size.x >= 0);
    ASSERT(size.y >= 0);
    ASSERT(size.x * size.y >= 0);

    i32 len = size.x * size.y;
    img->size = size;
    img->colors = perm_calloc(sizeof(img->colors[0]) * len);
    img->albedos = perm_calloc(sizeof(img->albedos[0]) * len);
    img->normals = perm_calloc(sizeof(img->normals[0]) * len);
}

void trace_img_del(trace_img_t* img)
{
    if (img)
    {
        pim_free(img->colors);
        pim_free(img->albedos);
        pim_free(img->normals);
        memset(img, 0, sizeof(*img));
    }
}

pt_scene_t* pt_scene_new(struct tables_s* tables, i32 maxDepth)
{
    table_t* drawTable = Drawables_Get(tables);
    if (!drawTable)
    {
        return NULL;
    }

    pt_scene_t* scene = perm_calloc(sizeof(*scene));
    float4 sceneMin = f4_s(1 << 20);
    float4 sceneMax = f4_s(-(1 << 20));
    i32 matCount = 0;
    i32 vertCount = 0;

    {
        const i32 drawCount = table_width(drawTable);
        const drawable_t* drawables = table_row(drawTable, TYPE_ARGS(drawable_t));
        const localtoworld_t* l2ws = table_row(drawTable, TYPE_ARGS(localtoworld_t));

        float4* positions = NULL;
        float4* normals = NULL;
        float2* uvs = NULL;
        material_t* materials = NULL;

        for (i32 i = 0; i < drawCount; ++i)
        {
            mesh_t mesh;
            if (mesh_get(drawables[i].mesh, &mesh))
            {
                const i32 vertBack = vertCount;
                const i32 matBack = matCount;
                vertCount += mesh.length;
                matCount += 1;

                const float4x4 M = l2ws[i].Value;
                const float4x4 IM = f4x4_inverse(f4x4_transpose(M));
                const material_t material = drawables[i].material;

                positions = perm_realloc(positions, sizeof(positions[0]) * vertCount);
                normals = perm_realloc(normals, sizeof(normals[0]) * vertCount);
                uvs = perm_realloc(uvs, sizeof(uvs[0]) * vertCount);
                materials = perm_realloc(materials, sizeof(materials[0]) * matCount);

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

                materials[matBack] = material;
            }
        }

        scene->vertCount = vertCount;
        scene->positions = positions;
        scene->normals = normals;
        scene->uvs = uvs;

        scene->matCount = matCount;
        scene->materials = materials;
    }

    {
        const i32 nodeCount = CalcNodeCount(maxDepth);
        scene->nodeCount = nodeCount;
        scene->boxes = perm_calloc(sizeof(scene->boxes[0]) * nodeCount);;
        scene->trilists = perm_calloc(sizeof(scene->trilists[0]) * nodeCount);
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
        }
        pim_free(scene->trilists);
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

    // test lights
    {
        const lights_t* lights = lights_get();
        const i32 ptCount = lights->ptCount;
        const pt_light_t* ptLights = lights->ptLights;
        for (i32 i = 0; i < ptCount; ++i)
        {
            sphere_t sph = { ptLights[i].pos };
            float t = isectSphere3D(ray, sph);
            if (t < e || t > hit.wuvt.w)
            {
                continue;
            }
            hit.type = hit_ptlight;
            hit.index = i;
            hit.wuvt.w = t;
        }
    }

    // test triangles
    {
        const i32* list = scene->trilists[n];
        const i32 len = SublistLen(list);
        const float4* pim_noalias vertices = scene->positions;

        for (i32 i = 0; i < len; ++i)
        {
            const i32 j = SublistGet(list, i);

            float4 tri = isectTri3D(ray, vertices[j + 0], vertices[j + 1], vertices[j + 2]);
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

pim_inline float VEC_CALL LambertianPdf(float4 N, float4 dir)
{
    return f1_max(0.0f, f4_dot3(N, dir) / kPi);
}

pim_inline float VEC_CALL GGXPdf(float NoH, float HoV, float roughness)
{
    float d = GGX_D(NoH, roughness);
    return (d * NoH) / f1_max(f16_eps, (4.0f * HoV));
}

pim_inline float VEC_CALL ScatterLambertian(
    prng_t* rng,
    float4 dirIn,
    const surfhit_t* surf,
    float4* dirOut)
{
    float4 N = surf->N;
    float4 dir = TanToWorld(N, SampleCosineHemisphere(f2_rand(rng)));
    *dirOut = dir;
    return LambertianPdf(N, dir);
}

pim_inline float VEC_CALL ScatterGGX(
    prng_t* rng,
    float4 dirIn,
    const surfhit_t* surf,
    float4* dirOut)
{
    float4 V = f4_neg(dirIn);
    float4 N = surf->N;
    float roughness = surf->roughness;

    float4 H = SampleGGXMicrofacet(f2_rand(rng), roughness);
    H = TanToWorld(N, H);

    float NoH = f1_max(0.0f, f4_dot3(N, H));
    float HoV = f1_max(0.0f, f4_dot3(H, V));

    float4 dir = f4_reflect3(dirIn, H);
    ASSERT(IsUnitLength(dir));
    *dirOut = dir;

    float pdf = GGXPdf(NoH, HoV, roughness);
    return pdf;
}

pim_inline float VEC_CALL Scatter(
    prng_t* rng,
    float4 dirIn,
    const surfhit_t* surf,
    float4* dirOut)
{
    if (prng_bool(rng))
    {
        return ScatterLambertian(rng, dirIn, surf, dirOut);
    }
    return ScatterGGX(rng, dirIn, surf, dirOut);
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
    default:
        ASSERT(false);
        break;
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
        pt_light_t light = lights_get_pt(hit.index);
        surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
        surf.N = f4_normalize3(f4_sub(surf.P, light.pos));
        surf.albedo = f4_1;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = light.rad;
    }
    break;
    }

    return surf;
}

pim_optimize
pt_result_t VEC_CALL pt_trace_ray(
    struct prng_s* rng,
    const pt_scene_t* scene,
    ray_t ray,
    i32 bounces)
{
    float4 albedo = f4_0;
    float4 normal = f4_0;
    float4 light = f4_0;
    float4 attenuation = f4_1;
    float pdf = 1.0f;

    for (i32 b = 0; b < bounces; ++b)
    {
        rayhit_t hit = TraceRay(scene, 0, ray, f4_rcp(ray.rd), 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }

        surfhit_t surf = GetSurface(scene, ray, hit);
        if (b == 0)
        {
            albedo = surf.albedo;
            normal = surf.N;
        }

        float4 emission = surf.emission;
        emission = f4_divvs(emission, pdf);
        emission = f4_mul(emission, attenuation);
        light = f4_add(light, emission);

        float4 dirOut;
        pdf = Scatter(rng, ray.rd, &surf, &dirOut);
        if (pdf > 0.0f)
        {
            ray.ro = surf.P;
            ray.rd = dirOut;

            float4 newAtten = surf.albedo;
            newAtten = f4_mulvs(newAtten, pdf);
            if (b >= 3)
            {
                float p = f4_hmax3(newAtten);
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

    pt_result_t result;
    result.albedo = f4_f3(albedo);
    result.normal = f4_f3(normal);
    result.color = f4_f3(light);
    return result;
}

pim_optimize
static void TraceFn(task_t* task, i32 begin, i32 end)
{
    trace_task_t* trace = (trace_task_t*)task;

    const pt_scene_t* scene = trace->scene;

    float3* pim_noalias colors = trace->img.colors;
    float3* pim_noalias albedos = trace->img.albedos;
    float3* pim_noalias normals = trace->img.normals;

    const int2 size = trace->img.size;
    const float sampleWeight = trace->sampleWeight;
    const i32 bounces = trace->bounces;
    const camera_t camera = trace->camera;
    const quat rot = camera.rotation;
    const float4 ro = camera.position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);

    prng_t rng = prng_get();

    const float2 rcpSize = f2_rcp(i2_f2(size));
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        float2 Xi = f2_tent(f2_rand(&rng));
        Xi = f2_mul(Xi, rcpSize);

        float2 uv = CoordToUv(size, coord);
        uv = f2_add(uv, Xi);
        uv = f2_snorm(uv);

        float4 rd = proj_dir(right, up, fwd, slope, uv);
        ray_t ray = { ro, rd };
        pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);

        colors[i] = f3_lerp(colors[i], result.color, sampleWeight);
        if (albedos)
        {
            albedos[i] = f3_lerp(albedos[i], result.albedo, sampleWeight);
        }
        if (normals)
        {
            normals[i] = f3_lerp(normals[i], result.normal, sampleWeight);
        }
    }

    prng_set(rng);
}

task_t* pt_trace(pt_trace_t* desc)
{
    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(desc->camera);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->scene = desc->scene;
    task->camera = desc->camera[0];
    task->img = desc->img;
    task->sampleWeight = desc->sampleWeight;
    task->bounces = desc->bounces;

    const i32 workSize = desc->img.size.x * desc->img.size.y;
    task_submit((task_t*)task, TraceFn, workSize);
    return (task_t*)task;
}

static void RayGenFn(task_t* pBase, i32 begin, i32 end)
{
    pt_raygen_t* task = (pt_raygen_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const pt_dist_t dist = task->dist;
    const i32 bounces = task->bounces;
    const ray_t origin = task->origin;
    float3x3 TBN;
    if (dist != ptdist_sphere)
    {
        TBN = NormalToTBN(origin.rd);
    }

    float3* pim_noalias colors = task->colors;
    float3* pim_noalias albedos = task->albedos;
    float3* pim_noalias normals = task->normals;
    float4* pim_noalias directions = task->directions;

    ray_t ray;
    ray.ro = origin.ro;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        float2 Xi = f2_rand(&rng);
        switch (dist)
        {
        default:
        case ptdist_sphere:
            ray.rd = SampleUnitSphere(Xi);
            break;
        case ptdist_hemi:
            ray.rd = TbnToWorld(TBN, SampleUnitHemisphere(Xi));
            break;
        }

        directions[i] = ray.rd;

        pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);

        colors[i] = result.color;
        albedos[i] = result.albedo;
        normals[i] = result.normal;
    }
    prng_set(rng);
}

pt_raygen_t* pt_raygen(const pt_scene_t* scene, ray_t origin, pt_dist_t dist, i32 count, i32 bounces)
{
    ASSERT(scene);
    ASSERT(count >= 0);

    pt_raygen_t* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = tmp_malloc(sizeof(task->colors[0]) * count);
    task->albedos = tmp_malloc(sizeof(task->albedos[0]) * count);
    task->normals = tmp_malloc(sizeof(task->normals[0]) * count);
    task->directions = tmp_malloc(sizeof(task->directions[0]) * count);
    task->dist = dist;
    task->bounces = bounces;
    task_submit((task_t*)task, RayGenFn, count);

    return task;
}
