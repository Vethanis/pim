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

typedef struct ray_s
{
    float4 ro;
    float4 rd;
} ray_t;

typedef struct surfhit_s
{
    float4 P;
    float4 N;
    float4 albedo;
    float4 rome;
} surfhit_t;

static i32 CalcNodeCount(i32 maxDepth)
{
    i32 nodeCount = 0;
    for (i32 i = 0; i < maxDepth; ++i)
    {
        nodeCount += 1 << (3 * i);
    }
    return nodeCount;
}

pim_inline i32 GetChild(i32 parent, i32 i)
{
    return (parent << 3) | (i + 1);
}

pim_inline i32 GetParent(i32 i)
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
    return
        (sdBox3D(box.center, box.extents, A) <= 0.0f) &&
        (sdBox3D(box.center, box.extents, B) <= 0.0f) &&
        (sdBox3D(box.center, box.extents, C) <= 0.0f);
}

static bool InsertTriangle(
    const box_t* boxes,
    i32** lists,
    int2* lenpops,
    const float4* vertices,
    i32 iVert,
    i32 n,
    i32 nodeCount)
{
    if (n < nodeCount)
    {
        if (BoxHoldsTri(boxes[n], vertices[iVert + 0], vertices[iVert + 1], vertices[iVert + 2]))
        {
            lenpops[n].y += 1;
            for (i32 i = 0; i < 8; ++i)
            {
                if (InsertTriangle(boxes, lists, lenpops, vertices, iVert, GetChild(n, i), nodeCount))
                {
                    return true;
                }
            }
            i32 len = 1 + lenpops[n].x;
            lenpops[n].x = len;
            lists[n] = perm_realloc(lists[n], sizeof(lists[n][0]) * len);
            lists[n][len - 1] = iVert;
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
        // scale bounds up a little bit, to account for fp precision
        // abs required when scene is not centered at origin
        sceneMax = f4_add(sceneMax, f4_mulvs(f4_abs(sceneMax), 0.01f));
        sceneMin = f4_sub(sceneMin, f4_mulvs(f4_abs(sceneMin), 0.01f));
        box_t bounds;
        bounds.center = f4_lerp(sceneMin, sceneMax, 0.5f);
        bounds.extents = f4_sub(sceneMax, bounds.center);

        const i32 nodeCount = CalcNodeCount(maxDepth);
        const i32 root = 0;

        box_t* boxes = perm_calloc(sizeof(boxes[0]) * nodeCount);
        i32** lists = perm_calloc(sizeof(lists[0]) * nodeCount);
        int2* lenpops = perm_calloc(sizeof(lenpops[0]) * nodeCount);

        boxes[root] = bounds;
        SetupBounds(boxes, root, nodeCount);

        for (i32 i = 0; (i + 3) <= vertCount; i += 3)
        {
            if (!InsertTriangle(boxes, lists, lenpops, positions, i, root, nodeCount))
            {
                ASSERT(false);
            }
        }

        scene->nodeCount = nodeCount;
        scene->boxes = boxes;
        scene->lists = lists;
        scene->lenpops = lenpops;
    }

    {
        const u32 lightQuery = (1 << CompId_Light) | (1 << CompId_Translation);
        i32 lightCount = 0;
        ent_t* lightEnts = ecs_query(lightQuery, 0, &lightCount);
        float4* lightPos = perm_malloc(sizeof(lightPos[0]) * lightCount);
        float4* lightRad = perm_malloc(sizeof(lightRad[0]) * lightCount);

        for (i32 i = 0; i < lightCount; ++i)
        {
            ent_t ent = lightEnts[i];
            const light_t* light = ecs_get(ent, CompId_Light);
            const translation_t* translation = ecs_get(ent, CompId_Translation);
            lightPos[i] = translation->Value;
            lightRad[i] = light->radiance;
        }

        scene->lightCount = lightCount;
        scene->lightPos = lightPos;
        scene->lightRad = lightRad;
    }

    return scene;
}

void pt_scene_del(pt_scene_t* scene)
{
    ASSERT(scene);

    pim_free(scene->positions);
    pim_free(scene->normals);
    pim_free(scene->uvs);

    pim_free(scene->materials);

    pim_free(scene->lightPos);
    pim_free(scene->lightRad);

    pim_free(scene->boxes);
    for (i32 i = 0; i < scene->nodeCount; ++i)
    {
        pim_free(scene->lists[i]);
    }
    pim_free(scene->lists);
    pim_free(scene->lenpops);

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

pim_optimize
static i32 VEC_CALL TraceRay(
    const pt_scene_t* scene,
    i32 n,
    ray_t ray,
    float4 rcpRd,
    float t,
    float4* wuvtOut)
{
    if ((n >= scene->nodeCount) || (scene->lenpops[n].y == 0))
    {
        return -1;
    }

    // test box
    const float2 nearFar = isectBox3D(ray.ro, rcpRd, scene->boxes[n]);
    if ((nearFar.y <= nearFar.x) || // miss
        (nearFar.x > t) || // further away, culled
        (nearFar.y < 0.0f)) // behind eye
    {
        return -1;
    }

    i32 index = -1;
    float4 wuvt = f4_s(-1.0f);

    // test triangles
    const i32* list = scene->lists[n];
    const i32 len = scene->lenpops[n].x;
    const float4* pim_noalias vertices = scene->positions;
    for (i32 i = 0; i < len; ++i)
    {
        const i32 j = list[i];

        float4 tri = isectTri3D(
            ray.ro, ray.rd, vertices[j + 0], vertices[j + 1], vertices[j + 2]);
        if (b4_any(f4_ltvs(tri, 0.0f)) || (tri.w > t))
        {
            continue;
        }
        t = tri.w;
        wuvt = tri;
        index = j;
    }

    // test children
    for (i32 i = 0; i < 8; ++i)
    {
        float4 tri;
        i32 ci = TraceRay(scene, GetChild(n, i), ray, rcpRd, t, &tri);
        if (ci != -1)
        {
            if (tri.w < t)
            {
                t = tri.w;
                wuvt = tri;
                index = ci;
            }
        }
    }

    *wuvtOut = wuvt;
    return index;
}

pim_inline float2 VEC_CALL prng_float2(prng_t* rng)
{
    return f2_v(prng_f32(rng), prng_f32(rng));
}

pim_inline float4 VEC_CALL RandomInUnitSphere(prng_t* rng)
{
    return SampleUnitSphere(prng_float2(rng));
}

pim_inline float4 VEC_CALL GetVert4(const float4* vertices, i32 iVert, float4 wuvt)
{
    return f4_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuvt);
}

pim_inline float2 VEC_CALL GetVert2(const float2* vertices, i32 iVert, float4 wuvt)
{
    return f2_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuvt);
}

pim_inline const material_t* VEC_CALL GetMaterial(const pt_scene_t* scene, i32 iVert)
{
    i32 iMat = (i32)(scene->normals[iVert].w);
    ASSERT(iMat >= 0);
    ASSERT(iMat < scene->matCount);
    return scene->materials + iMat;
}

pim_inline float4 VEC_CALL GetAlbedo(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatAlbedo);
    texture_t tex;
    if (texture_get(mat->albedo, &tex))
    {
        value = f4_mul(value, Tex_Bilinearf2(tex, uv));
    }
    return value;
}

pim_inline float4 VEC_CALL GetRome(const material_t* mat, float2 uv)
{
    float4 value = ColorToLinear(mat->flatRome);
    texture_t tex;
    if (texture_get(mat->rome, &tex))
    {
        value = f4_mul(value, Tex_Bilinearf2(tex, uv));
    }
    return value;
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
    float4 dir = f4_add(R, f4_mulvs(RandomInUnitSphere(rng), surf->rome.x));
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
    if (surf->rome.z < 0.5f)
    {
        return ScatterLambertian(rng, rin, surf, attenuation, scattered);
    }
    return ScatterMetallic(rng, rin, surf, attenuation, scattered);
}

static void VEC_CALL GetSurface(
    const pt_scene_t* scene,
    i32 iVert,
    float4 wuvt,
    surfhit_t* surf)
{
    ASSERT(iVert < scene->vertCount);
    const material_t* mat = GetMaterial(scene, iVert);
    const float2 uv = GetVert2(scene->uvs, iVert, wuvt);
    surf->P = GetVert4(scene->positions, iVert, wuvt);
    surf->N = f4_normalize3(GetVert4(scene->normals, iVert, wuvt));
    surf->albedo = GetAlbedo(mat, uv);
    surf->rome = GetRome(mat, uv);
    // offset to avoid self-intersection
    surf->P = f4_add(surf->P, f4_mulvs(surf->N, 0.001f));
}

pim_optimize
static float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    const surfhit_t* surf)
{
    float4 reflected = f4_s(0.0f);

    const i32 lightCount = scene->lightCount;
    const float4* pim_noalias lightPositions = scene->lightPos;
    const float4* pim_noalias lightRadiances = scene->lightRad;
    for (i32 i = 0; i < lightCount; ++i)
    {
        float4 L = f4_sub(lightPositions[i], surf->P);
        float lDist = f4_length3(L);
        L = f4_divvs(L, lDist);
        float cosTheta = f4_dot3(L, surf->N);
        if (cosTheta > 0.0f)
        {
            float4 wuvt;
            ray_t ray = { surf->P, L };
            i32 iRay = TraceRay(
                scene,
                0,
                ray,
                f4_rcp(ray.rd),
                lDist,
                &wuvt);
            if (iRay == -1)
            {
                float attenuation = cosTheta / (lDist*lDist);
                reflected = f4_add(
                    reflected, f4_mulvs(lightRadiances[i], attenuation));
            }
        }
    }

    return f4_mul(reflected, surf->albedo);
}

pim_optimize
static float4 VEC_CALL TracePixel(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray,
    i32 b,
    i32 bounces)
{
    float4 light = f4_s(0.0f);
    if (b > bounces)
    {
        return light;
    }

    float4 wuvt;
    i32 iVert = TraceRay(scene, 0, ray, f4_rcp(ray.rd), 1 << 20, &wuvt);
    if (iVert >= 0)
    {
        surfhit_t surf;
        GetSurface(scene, iVert, wuvt, &surf);

        light = f4_add(light, SampleLights(scene, &surf));

        ray_t scattered;
        float4 attenuation;
        if (Scatter(rng, ray, &surf, &attenuation, &scattered))
        {
            float4 incoming = TracePixel(rng, scene, scattered, b + 1, bounces);
            light = f4_add(light, f4_mul(incoming, attenuation));
        }
    }

    return light;
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

    prng_t rng = prng_create();

    const float2 dSize = f2_rcp(i2_f2(size));
    for (i32 i = begin; i < end; ++i)
    {
        const int2 xy = { i % size.x, i / size.x };
        const float2 jitter = { prng_f32(&rng), prng_f32(&rng) };
        const float2 coord = f2_snorm(f2_mul(f2_add(i2_f2(xy), jitter), dSize));
        const float4 rd = proj_dir(right, up, fwd, slope, coord);
        ray_t ray = { ro, rd };
        float4 sample = TracePixel(&rng, scene, ray, 0, bounces);
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
