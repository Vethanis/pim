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
#include "rendering/drawable.h"
#include "threading/task.h"
#include "common/random.h"
#include "common/profiler.h"

#include <string.h>

typedef enum
{
    hit_nothing = 0,
    hit_triangle,
    hit_light,

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

typedef struct scatter_s
{
    float4 dir;
    float4 attenuation;
    float pdf;
    bool specularBounce;
} scatter_t;

typedef struct trace_task_s
{
    task_t task;
    pt_trace_t trace;
    camera_t camera;
} trace_task_t;

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    ray_t origin;
    float4* colors;
    float4* directions;
    pt_dist_t dist;
    i32 bounces;
} pt_raygen_t;

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

static bool InsertLight(
    pt_scene_t* scene,
    i32 iLight,
    i32 n)
{
    if (n < scene->nodeCount)
    {
        if (BoxHoldsSph(
            scene->boxes[n],
            scene->lights[iLight].value))
        {
            scene->pops[n] += 1;
            for (i32 i = 0; i < 8; ++i)
            {
                if (InsertLight(
                    scene,
                    iLight,
                    GetChild(n, i)))
                {
                    return true;
                }
            }
            scene->lightLists[n] = SublistPush(scene->lightLists[n], iLight);
            return true;
        }
    }
    return false;
}

pim_inline bool VEC_CALL i2_edgefn(int2 a, int2 b, int2 c)
{
    int2 e1 = i2_sub(b, a);
    int2 e2 = i2_sub(c, a);
    i32 t1 = e2.x * e1.y;
    i32 t2 = e2.y * e1.x;
    return (t1 * t2) >= 0;
}

pim_inline bool VEC_CALL i2_inside(int2 a, int2 b, int2 c, int2 p)
{
    return i2_edgefn(a, b, p) && i2_edgefn(b, c, p) && i2_edgefn(c, a, p);
}

static bool VEC_CALL IsMaterialEmissive(material_t mat)
{
    float4 flatAlbedo = ColorToLinear(mat.flatAlbedo);
    float flatEmission = ColorToLinear(mat.flatRome).w;
    if (f4_perlum(UnpackEmission(flatAlbedo, flatEmission)) < 0.01f)
    {
        return false;
    }

    texture_t albedoMap = { 0 };
    if (!texture_get(mat.albedo, &albedoMap))
    {
        return false;
    }

    const int2 size = albedoMap.size;
    const u32* pim_noalias texels = albedoMap.texels;
    ASSERT(texels);

    const i32 numTexels = size.x * size.y;
    for (i32 j = 0; j < numTexels; ++j)
    {
        float4 albedo = ColorToLinear(texels[j]);
        albedo = f4_mul(flatAlbedo, albedo);
        float4 emission = UnpackEmission(albedo, flatEmission);
        float lum = f4_perlum(emission);
        if (lum >= 0.1f)
        {
            return true;
        }
    }

    return false;
}

typedef struct empt_s
{
    float4 position;
    float4 normal;
    float4 emission;
} empt_t;

static float VEC_CALL empt_dist(empt_t a, empt_t b)
{
    return f4_distance3(a.position, b.position);
    //float aLum = f4_perlum(a.emission);
    //float bLum = f4_perlum(b.emission);
    //float lumDist = f1_distance(aLum, bLum);
    //float cosTheta = f4_dot3(a.normal, b.normal);
    //float norDist = f1_distance(1.0f, cosTheta);
    //float dist = posDist + lumDist + norDist;
    //return dist;
}

static empt_t VEC_CALL ClusterMean(
    const empt_t* pim_noalias points,
    i32 ptCount,
    const i32* pim_noalias nearests,
    i32 iMean)
{
    empt_t mean = { f4_0, f4_0, f4_0 };
    i32 population = 0;
    for (i32 i = 0; i < ptCount; ++i)
    {
        if (nearests[i] == iMean)
        {
            ++population;
        }
    }
    if (population > 0)
    {
        const float scale = 1.0f / population;
        for (i32 i = 0; i < ptCount; ++i)
        {
            if (nearests[i] == iMean)
            {
                empt_t pt = points[i];
                mean.position = f4_add(mean.position, f4_mulvs(pt.position, scale));
                mean.normal = f4_add(mean.normal, f4_mulvs(pt.normal, scale));
                mean.emission = f4_add(mean.emission, f4_mulvs(pt.emission, scale));
            }
        }
        mean.normal = f4_normalize3(mean.normal);
        float radius = 0.0f;
        for (i32 i = 0; i < ptCount; ++i)
        {
            if (nearests[i] == iMean)
            {
                float dist = f4_distance3(mean.position, points[i].position);
                radius = f1_max(radius, dist);
            }
        }
        mean.position.w = radius;
    }
    return mean;
}

static i32 VEC_CALL ClusterNearest(
    i32 meanCount,
    const empt_t* pim_noalias means,
    empt_t pt)
{
    i32 chosen = -1;
    float chosenDist = 1 << 20;
    for (i32 i = 0; i < meanCount; ++i)
    {
        float dist = empt_dist(pt, means[i]);
        if (dist < chosenDist)
        {
            chosen = i;
            chosenDist = dist;
        }
    }
    return chosen;
}

typedef struct task_ClusterNearest
{
    task_t task;
    const empt_t* means;
    const empt_t* points;
    i32* nearests;
    i32 numMeans;
    i32 numPoints;
} task_ClusterNearest;

static void ClusterNearestFn(task_t* pbase, i32 begin, i32 end)
{
    task_ClusterNearest* task = (task_ClusterNearest*)pbase;
    const empt_t* pim_noalias means = task->means;
    const empt_t* pim_noalias points = task->points;
    const i32 numMeans = task->numMeans;
    const i32 numPoints = task->numPoints;
    i32* pim_noalias nearests = task->nearests;

    for (i32 i = begin; i < end; ++i)
    {
        nearests[i] = ClusterNearest(numMeans, means, points[i]);
    }
}

typedef struct task_ClusterMean
{
    task_t task;
    empt_t* means;
    const empt_t* points;
    const i32* nearests;
    i32 ptCount;
} task_ClusterMean;

static void ClusterMeanFn(task_t* pbase, i32 begin, i32 end)
{
    task_ClusterMean* task = (task_ClusterMean*)pbase;
    empt_t* pim_noalias means = task->means;
    const empt_t* pim_noalias points = task->points;
    const i32* pim_noalias nearests = task->nearests;
    const i32 ptCount = task->ptCount;

    for (i32 i = begin; i < end; ++i)
    {
        means[i] = ClusterMean(points, ptCount, nearests, i);
    }
}

static float VEC_CALL ClusterCost(
    const empt_t* pim_noalias points,
    i32 ptCount,
    const i32* pim_noalias nearests,
    i32 iMean,
    empt_t mean)
{
    float cost = 0.0f;
    i32 population = 0;
    for (i32 i = 0; i < ptCount; ++i)
    {
        if (nearests[i] == iMean)
        {
            float dist = empt_dist(points[i], mean);
            cost = f1_max(cost, dist);
        }
    }
    return cost;
}

typedef struct task_ClusterCost
{
    task_t task;
    const empt_t* points;
    i32 ptCount;
    const i32* nearests;
    const empt_t* means;
    float* costs;
} task_ClusterCost;

static void ClusterCostFn(task_t* pbase, i32 begin, i32 end)
{
    task_ClusterCost* task = (task_ClusterCost*)pbase;
    const empt_t* points = task->points;
    const i32 ptCount = task->ptCount;
    const i32* nearests = task->nearests;
    const empt_t* means = task->means;
    float* costs = task->costs;

    for (i32 i = begin; i < end; ++i)
    {
        costs[i] = ClusterCost(points, ptCount, nearests, i, means[i]);
    }
}

static empt_t* ClusterPoints(i32 ptCount, const empt_t* points, i32 k, float* costOut)
{
    empt_t* pim_noalias means = perm_calloc(sizeof(means[0]) * k);
    empt_t* pim_noalias prevMeans = perm_calloc(sizeof(prevMeans[0]) * k);
    float* pim_noalias costs = perm_calloc(sizeof(costs[0]) * k);
    i32* pim_noalias nearests = perm_calloc(sizeof(nearests[0]) * ptCount);

    prng_t rng = prng_get();
    for (i32 i = 0; i < k; ++i)
    {
        i32 j = prng_i32(&rng) % ptCount;
        empt_t pt = points[j];
        means[i] = pt;
    }
    prng_set(rng);

    task_ClusterNearest* clusterNearest = tmp_calloc(sizeof(*clusterNearest));
    clusterNearest->means = means;
    clusterNearest->nearests = nearests;
    clusterNearest->points = points;
    clusterNearest->numMeans = k;
    clusterNearest->numPoints = ptCount;

    task_ClusterMean* clusterMean = tmp_calloc(sizeof(*clusterMean));
    clusterMean->means = means;
    clusterMean->nearests = nearests;
    clusterMean->points = points;
    clusterMean->ptCount = ptCount;

    task_ClusterCost* clusterCost = tmp_calloc(sizeof(*clusterCost));
    clusterCost->costs = costs;
    clusterCost->means = means;
    clusterCost->nearests = nearests;
    clusterCost->points = points;
    clusterCost->ptCount = ptCount;

    i32 iterations = 0;
    do
    {
        // associate each node with nearest mean
        task_run(&clusterNearest->task, ClusterNearestFn, ptCount);

        // update means
        for (i32 i = 0; i < k; ++i)
        {
            prevMeans[i] = means[i];
        }
        task_run(&clusterMean->task, ClusterMeanFn, k);

        ++iterations;
        if (iterations > 10)
        {
            task_run(&clusterCost->task, ClusterCostFn, k);
            float cost = 0.0f;
            for (i32 i = 0; i < k; ++i)
            {
                cost = f1_max(cost, costs[i]);
            }
            if (cost > 1.0f)
            {
                break;
            }
        }

    } while (memcmp(means, prevMeans, sizeof(means[0]) * k));

    task_run(&clusterCost->task, ClusterCostFn, k);
    float cost = 0.0f;
    for (i32 i = 0; i < k; ++i)
    {
        cost = f1_max(cost, costs[i]);
    }
    *costOut = cost;

    pim_free(costs);
    pim_free(prevMeans);
    pim_free(nearests);

    return means;
}

static void CreateLights(
    pt_scene_t* scene,
    i32 ptCount,
    const empt_t* points,
    float maxRadius)
{
    if (ptCount < 1)
    {
        return;
    }

    float prevCost = 1 << 20;
    float curCost = prevCost;
    i32 k = 2;

    empt_t* means = NULL;
recluster:
    means = ClusterPoints(ptCount, points, k, &curCost);
    if (curCost > maxRadius)
    {
        pim_free(means);
        k += k / 2;
        prevCost = curCost;
        goto recluster;
    }

    sphere_t* spheres = perm_malloc(sizeof(spheres[0]) * k);
    float4* rads = perm_malloc(sizeof(rads[0]) * k);
    for (i32 i = 0; i < k; ++i)
    {
        empt_t mean = means[i];
        spheres[i].value = mean.position;
        rads[i] = mean.emission;
    }
    pim_free(means);

    scene->lights = spheres;
    scene->lightRads = rads;
    scene->lightCount = k;
}

static empt_t* CreateEmissionPoints(
    const pt_scene_t* scene,
    i32* countOut,
    float threshold)
{
    empt_t* points = NULL;
    i32 ptCount = 0;

    const i32 vertCount = scene->vertCount;
    const float4* pim_noalias positions = scene->positions;
    const float4* pim_noalias normals = scene->normals;
    const float2* pim_noalias uvs = scene->uvs;
    const i32* pim_noalias matIds = scene->matIds;

    const i32 matCount = scene->matCount;
    const material_t* pim_noalias mats = scene->materials;

    i32 prevNonEmissiveMatId = -1;
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        const i32 a = i + 0;
        const i32 b = i + 1;
        const i32 c = i + 2;

        const i32 matId = matIds[i];
        if (matId == prevNonEmissiveMatId)
        {
            continue;
        }

        const material_t mat = mats[matId];
        if (!IsMaterialEmissive(mat))
        {
            prevNonEmissiveMatId = matId;
            continue;
        }

        texture_t albedoMap = { 0 };
        if (!texture_get(mat.albedo, &albedoMap))
        {
            continue;
        }

        float2 UA = uvs[a];
        float2 UB = uvs[b];
        float2 UC = uvs[c];

        const float wLen = f1_max(f2_distance(UB, UA), f2_distance(UB, UC));
        if (wLen < kEpsilon)
        {
            continue;
        }

        const int2 size = albedoMap.size;
        const i32 maxExtent = i1_max(size.x, size.y);
        const u32* pim_noalias texels = albedoMap.texels;
        ASSERT(texels);

        float4 flatAlbedo = ColorToLinear(mat.flatAlbedo);
        float flatEmission = ColorToLinear(mat.flatRome).w;

        float4 A = positions[a];
        float4 B = positions[b];
        float4 C = positions[c];
        float4 NA = normals[a];
        float4 NB = normals[b];
        float4 NC = normals[c];

        const float dw = 1.0f / (wLen * maxExtent);
        for (float w = 0.0f; w <= 1.0f; w += dw)
        {
            float2 p1 = f2_lerp(UA, UB, w);
            float2 p2 = f2_lerp(UC, UB, w);
            float uLen = f2_distance(p1, p2);
            if (uLen < kEpsilon)
            {
                continue;
            }
            const float du = 1.0f / (uLen * maxExtent);
            for (float u = 0.0f; u <= 1.0f; u += du)
            {
                if ((w + u) > 1.0f)
                {
                    u = 2.0f;
                    continue;
                }
                float2 uv = f2_lerp(p1, p2, u);
                float4 albedo = UvBilinearWrap_c32(texels, size, uv);
                albedo = f4_mul(albedo, flatAlbedo);
                float4 emission = UnpackEmission(albedo, flatEmission);
                float lum = f4_perlum(emission);
                if (lum < threshold)
                {
                    continue;
                }

                float4 position = f4_lerpvs(f4_lerpvs(A, B, w), f4_lerpvs(C, B, w), u);
                float4 normal = f4_lerpvs(f4_lerpvs(NA, NB, w), f4_lerpvs(NC, NB, w), u);
                normal = f4_normalize3(normal);

                const empt_t empt =
                {
                    .position = position,
                    .normal = normal,
                    .emission = emission,
                };

                ++ptCount;
                PermReserve(points, ptCount);
                points[ptCount - 1] = empt;
            }
        }
    }

    *countOut = ptCount;
    return points;
}

static void FlattenDrawables(pt_scene_t* scene)
{
    const drawables_t* drawTable = drawables_get();
    const i32 drawCount = drawTable->count;
    const meshid_t* meshes = drawTable->meshes;
    const float4x4* matrices = drawTable->matrices;
    const material_t* materials = drawTable->materials;

    i32 vertCount = 0;
    float4* positions = NULL;
    float4* normals = NULL;
    float2* uvs = NULL;
    i32* matIds = NULL;

    i32 matCount = 0;
    material_t* sceneMats = NULL;

    for (i32 i = 0; i < drawCount; ++i)
    {
        mesh_t mesh;
        if (mesh_get(meshes[i], &mesh))
        {
            const i32 vertBack = vertCount;
            const i32 matBack = matCount;
            vertCount += mesh.length;
            matCount += 1;

            const float4x4 M = matrices[i];
            const float3x3 IM = f3x3_IM(M);
            const material_t material = materials[i];

            PermReserve(positions, vertCount);
            PermReserve(normals, vertCount);
            PermReserve(uvs, vertCount);
            PermReserve(matIds, vertCount);
            PermReserve(sceneMats, matCount);

            sceneMats[matBack] = material;

            for (i32 j = 0; j < mesh.length; ++j)
            {
                positions[vertBack + j] = f4x4_mul_pt(M, mesh.positions[j]);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                normals[vertBack + j] = f4_normalize3(f3x3_mul_col(IM, mesh.normals[j]));
            }

            for (i32 j = 0; (j + 3) <= mesh.length; j += 3)
            {
                float2 UA = TransformUv(mesh.uvs[j + 0], material.st);
                float2 UB = TransformUv(mesh.uvs[j + 1], material.st);
                float2 UC = TransformUv(mesh.uvs[j + 2], material.st);
                uvs[vertBack + j + 0] = UA;
                uvs[vertBack + j + 1] = UB;
                uvs[vertBack + j + 2] = UC;
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                matIds[vertBack + j] = matBack;
            }
        }
    }

    scene->vertCount = vertCount;
    scene->positions = positions;
    scene->normals = normals;
    scene->uvs = uvs;
    scene->matIds = matIds;

    scene->matCount = matCount;
    scene->materials = sceneMats;
}

static box_t VEC_CALL CalcSceneBounds(const pt_scene_t* scene)
{
    float4 sceneMin = f4_s(1 << 20);
    float4 sceneMax = f4_s(-(1 << 20));

    const i32 vertCount = scene->vertCount;
    const float4* pim_noalias positions = scene->positions;
    for (i32 i = 0; i < vertCount; ++i)
    {
        float4 position = positions[i];
        sceneMin = f4_min(sceneMin, position);
        sceneMax = f4_max(sceneMax, position);
    }

    // scale bounds up a little bit, to account for fp precision
    // abs required when scene is not centered at origin
    sceneMax = f4_add(sceneMax, f4_mulvs(f4_abs(sceneMax), 0.01f));
    sceneMin = f4_sub(sceneMin, f4_mulvs(f4_abs(sceneMin), 0.01f));
    box_t bounds;
    bounds.center = f4_lerpvs(sceneMin, sceneMax, 0.5f);
    bounds.extents = f4_sub(sceneMax, bounds.center);

    return bounds;
}

static void BuildBVH(pt_scene_t* scene, i32 maxDepth)
{
    const i32 nodeCount = CalcNodeCount(maxDepth);
    scene->nodeCount = nodeCount;
    scene->boxes = perm_calloc(sizeof(scene->boxes[0]) * nodeCount);;
    scene->trilists = perm_calloc(sizeof(scene->trilists[0]) * nodeCount);
    scene->lightLists = perm_calloc(sizeof(scene->lightLists[0]) * nodeCount);
    scene->pops = perm_calloc(sizeof(scene->pops[0]) * nodeCount);

    scene->boxes[0] = CalcSceneBounds(scene);
    SetupBounds(scene->boxes, 0, nodeCount);

    const i32 vertCount = scene->vertCount;
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        if (!InsertTriangle(scene, i, 0))
        {
            ASSERT(false);
        }
    }

    const i32 lightCount = scene->lightCount;
    for (i32 i = 0; i < lightCount; ++i)
    {
        if (!InsertLight(scene, i, 0))
        {
            ASSERT(false);
        }
    }
}

pt_scene_t* pt_scene_new(i32 maxDepth, bool nee, float emThresh, float maxDist)
{
    pt_scene_t* scene = perm_calloc(sizeof(*scene));
    scene->nextEventEstimation = nee;

    FlattenDrawables(scene);

    if (nee)
    {
        i32 ptCount = 0;
        empt_t* points = CreateEmissionPoints(scene, &ptCount, emThresh);
        CreateLights(scene, ptCount, points, maxDist);
        pim_free(points);
    }

    BuildBVH(scene, maxDepth);

    return scene;
}

void pt_scene_del(pt_scene_t* scene)
{
    if (scene)
    {
        pim_free(scene->positions);
        pim_free(scene->normals);
        pim_free(scene->uvs);
        pim_free(scene->matIds);

        pim_free(scene->materials);

        pim_free(scene->lights);
        pim_free(scene->lightRads);

        pim_free(scene->boxes);
        for (i32 i = 0; i < scene->nodeCount; ++i)
        {
            pim_free(scene->trilists[i]);
            pim_free(scene->lightLists[i]);
        }
        pim_free(scene->trilists);
        pim_free(scene->lightLists);
        pim_free(scene->pops);

        memset(scene, 0, sizeof(*scene));
        pim_free(scene);
    }
}

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

    const float kNearThresh = 0.0001f;
    // test box
    const float2 nearFar = isectBox3D(ray.ro, rcpRd, scene->boxes[n]);
    if ((nearFar.y <= nearFar.x) || // miss
        (nearFar.x > limit) || // further away, culled
        (nearFar.y < kNearThresh)) // behind eye
    {
        return hit;
    }

    // test lights
    {
        const i32* list = scene->lightLists[n];
        const i32 len = SublistLen(list);
        const sphere_t* pim_noalias lights = scene->lights;

        for (i32 i = 0; i < len; ++i)
        {
            const i32 j = SublistGet(list, i);

            float t = isectSphere3D(ray, lights[j]);
            if (t < kNearThresh || t > hit.wuvt.w)
            {
                continue;
            }
            hit.type = hit_light;
            hit.index = j;
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
            if (t < kNearThresh || t > hit.wuvt.w)
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
    i32 triIndex = hit.index;
    i32 matIndex = scene->matIds[triIndex];
    ASSERT(matIndex >= 0);
    ASSERT(matIndex < scene->matCount);
    return scene->materials + matIndex;
}

pim_inline float4 VEC_CALL SampleSpecular(
    prng_t* rng,
    float4 I,
    float4 N,
    float alpha)
{
    float4 H = TanToWorld(N, SampleGGXMicrofacet(f2_rand(rng), alpha));
    float4 L = f4_normalize3(f4_reflect3(I, H));
    return L;
}

pim_inline float4 VEC_CALL SampleDiffuse(prng_t* rng, float4 N)
{
    float4 L = TanToWorld(N, SampleCosineHemisphere(f2_rand(rng)));
    return L;
}

// samples a light direction of the brdf
pim_inline float4 VEC_CALL BrdfSample(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I)
{
    float4 N = surf->N;
    float metallic = surf->metallic;
    float alpha = BrdfAlpha(surf->roughness);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    const float specProb = 1.0f / (amtSpecular + amtDiffuse);

    float4 L;
    if (prng_f32(rng) < specProb)
    {
        L = SampleSpecular(rng, I, N, alpha);
    }
    else
    {
        L = SampleDiffuse(rng, N);
    }

    return L;
}

// returns pdf of the given interaction
pim_inline float VEC_CALL BrdfEvalPdf(
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL < kEpsilon)
    {
        return 0.0f;
    }

    float4 V = f4_neg(I);
    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);

    // metals are only specular
    const float amtDiffuse = 1.0f - surf->metallic;
    const float amtSpecular = 1.0f;
    const float rcpProb = 1.0f / (amtSpecular + amtDiffuse);

    float alpha = BrdfAlpha(surf->roughness);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = (diffusePdf + specularPdf) * rcpProb;

    return pdf;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL < kEpsilon)
    {
        return f4_0;
    }

    float4 V = f4_neg(I);
    float4 albedo = surf->albedo;
    float metallic = surf->metallic;
    float roughness = surf->roughness;
    float alpha = BrdfAlpha(roughness);
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);
    float NoV = f4_dotsat(N, V);

    float4 brdf = f4_0;
    {
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, f4_mulvs(Fr, amtSpecular));
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Lambert());
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }
    return f4_mulvs(brdf, NoL);
}

static scatter_t VEC_CALL BrdfScatter(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I)
{
    scatter_t result = { 0 };

    float4 N = surf->N;
    float4 V = f4_neg(I);
    float4 albedo = surf->albedo;
    float metallic = surf->metallic;
    float roughness = surf->roughness;
    float alpha = BrdfAlpha(roughness);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    const float rcpProb = 1.0f / (amtSpecular + amtDiffuse);

    bool specular = prng_f32(rng) < rcpProb;
    float4 L = specular ?
        SampleSpecular(rng, I, N, alpha) :
        SampleDiffuse(rng, N);

    float NoL = f4_dotsat(N, L);
    if (NoL < kEpsilon)
    {
        return result;
    }

    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);
    float NoV = f4_dotsat(N, V);

    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float pdf = rcpProb * (diffusePdf + specularPdf);

    if (pdf < kEpsilon)
    {
        return result;
    }

    float4 brdf = f4_0;
    {
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, f4_mulvs(Fr, amtSpecular));
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Lambert());
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(brdf, NoL);
    result.specularBounce = specular;

    return result;
}

static bool LightSelect(
    const pt_scene_t* scene,
    prng_t* rng,
    i32* idOut,
    float* weightOut)
{
    i32 lightCount = scene->lightCount;
    if (lightCount > 0)
    {
        *idOut = prng_i32(rng) % lightCount;
        *weightOut = 1.0f / lightCount;
        return true;
    }
    *idOut = -1;
    *weightOut = 0.0f;
    return false;
}

static float4 VEC_CALL LightSample(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    i32 iLight)
{
    float4 pos = scene->lights[iLight].value;
    float4 pt = SamplePointOnSphere(f2_rand(rng), pos, pos.w);
    return f4_normalize3(f4_sub(pt, surf->P));
}

static float VEC_CALL LightEvalPdf(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    i32 iLight,
    float4 L)
{
    ray_t ray = { surf->P, L };
    rayhit_t hit = TraceRay(scene, 0, ray, f4_rcp(L), 1 << 20);
    if ((hit.type != hit_light) || (hit.index != iLight))
    {
        return 0.0f;
    }

    float4 light = scene->lights[iLight].value;
    float t = hit.wuvt.w;
    float4 hitPt = f4_add(surf->P, f4_mulvs(L, t));
    float4 sphNorm = f4_normalize3(f4_sub(hitPt, light));
    float cosTheta = f1_abs(f4_dot3(sphNorm, L));
    float pdf = LightPdf(SphereArea(light.w), cosTheta, t * t);

    return pdf;
}

static scatter_t VEC_CALL LightScatter(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I)
{
    scatter_t result = { 0 };
    bool choseSelf =
        (hit->type == hit_light) &&
        (hit->index == iLight);
    if (!choseSelf)
    {
        float4 L = LightSample(scene, rng, surf, iLight);
        float lightPdf = LightEvalPdf(scene, rng, surf, iLight, L);
        result.dir = L;
        result.pdf = lightPdf;
        result.attenuation = BrdfEval(I, surf, L);
    }
    return result;
}

// scatter along brdf or towards a light
static scatter_t VEC_CALL MIScatter(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    scatter_t result = { 0 };

    float lightPdfWeight;
    i32 iLight;
    if (!LightSelect(scene, rng, &iLight, &lightPdfWeight))
    {
        return BrdfScatter(rng, surf, I);
    }

    if (prng_f32(rng) < 0.5f)
    {
        result = BrdfScatter(rng, surf, I);
        float lightPdf = LightEvalPdf(scene, rng, surf, iLight, result.dir);
        lightPdf *= lightPdfWeight;
        float weight = PowerHeuristic(1, result.pdf, 1, lightPdf);
        result.attenuation = f4_mulvs(result.attenuation, weight);
    }
    else
    {
        result = LightScatter(scene, rng, surf, hit, iLight, I);
        result.pdf *= lightPdfWeight;
        float brdfPdf = BrdfEvalPdf(I, surf, result.dir);
        float weight = PowerHeuristic(1, result.pdf, 1, brdfPdf);
        result.attenuation = f4_mulvs(result.attenuation, weight);
    }

    return result;
}

static float4 VEC_CALL EstimateDirect(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I)
{
    float4 direct = f4_0;
    float4 rad = scene->lightRads[iLight];

    if (prng_bool(rng))
    {
        // sample a light direction against the brdf
        scatter_t scatter = LightScatter(scene, rng, surf, hit, iLight, I);
        if (scatter.pdf > kEpsilon)
        {
            float brdfPdf = BrdfEvalPdf(I, surf, scatter.dir);
            if (brdfPdf > kEpsilon)
            {
                float weight = PowerHeuristic(1, scatter.pdf, 1, brdfPdf);
                scatter.attenuation = f4_mulvs(scatter.attenuation, weight);
                float4 Li = f4_divvs(
                    f4_mul(rad, scatter.attenuation),
                    scatter.pdf);
                direct = f4_add(direct, Li);
            }
        }
    }
    else
    {
        // sample a brdf direction against the light
        scatter_t scatter = BrdfScatter(rng, surf, I);
        if (scatter.pdf > kEpsilon)
        {
            float lightPdf = LightEvalPdf(scene, rng, surf, iLight, scatter.dir);
            if (lightPdf > kEpsilon)
            {
                float weight = PowerHeuristic(1, scatter.pdf, 1, lightPdf);
                scatter.attenuation = f4_mulvs(scatter.attenuation, weight);
                float4 Li = f4_divvs(
                    f4_mul(rad, scatter.attenuation),
                    scatter.pdf);
                direct = f4_add(direct, Li);
            }
        }
    }

    return direct;
}

static float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    float4 light = f4_0;
    i32 iLight;
    float lightPdfWeight;
    if (LightSelect(scene, rng, &iLight, &lightPdfWeight))
    {
        bool isSelf = (hit->type == hit_light) && (hit->index == iLight);
        if (!isSelf)
        {
            float4 direct = EstimateDirect(
                scene, rng, surf, hit, iLight, I);
            direct = f4_divvs(direct, lightPdfWeight);
            light = f4_add(light, direct);
        }
    }
    return light;
}

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
        float4 tN = material_normal(mat, uv);
        surf.N = TanToWorld(surf.N, tN);
        surf.albedo = material_albedo(mat, uv);
        float4 rome = material_rome(mat, uv);
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
        surf.emission = UnpackEmission(surf.albedo, rome.w);
    }
    break;
    case hit_light:
    {
        float4 lPos = scene->lights[hit.index].value;
        float4 lRad = scene->lightRads[hit.index];
        surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
        surf.N = f4_normalize3(f4_sub(surf.P, lPos));
        surf.albedo = f4_1;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = lRad;
    }
    break;
    }

    return surf;
}

static float4 VEC_CALL pt_trace_albedo(
    const pt_scene_t* scene,
    ray_t ray)
{
    rayhit_t hit = TraceRay(scene, 0, ray, f4_rcp(ray.rd), 1 << 20);
    if (hit.type != hit_nothing)
    {
        surfhit_t surf = GetSurface(scene, ray, hit);
        return surf.albedo;
    }
    return f4_0;
}

pt_result_t VEC_CALL pt_trace_ray(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray,
    i32 bounces)
{
    float4 albedo = f4_0;
    float4 normal = f4_0;
    float4 light = f4_0;
    float4 attenuation = f4_1;
    const bool nee = scene->nextEventEstimation;
    const bool rrBegin = nee ? 0 : 3;

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

        if (nee)
        {
            if (b == 0)
            {
                light = f4_add(light, f4_mul(surf.emission, attenuation));
            }
            float4 direct = SampleLights(scene, rng, &surf, &hit, ray.rd);
            light = f4_add(light, f4_mul(direct, attenuation));
        }
        else
        {
            light = f4_add(light, f4_mul(surf.emission, attenuation));
        }

        scatter_t scatter = BrdfScatter(rng, &surf, ray.rd);
        if (scatter.pdf < kEpsilon)
        {
            break;
        }

        attenuation = f4_mul(
            attenuation,
            f4_divvs(scatter.attenuation, scatter.pdf));

        ray.ro = surf.P;
        ray.rd = scatter.dir;

        if (b >= rrBegin)
        {
            float p = f4_perlum(attenuation);
            if (prng_f32(rng) < p)
            {
                attenuation = f4_divvs(attenuation, p);
            }
            else
            {
                break;
            }
        }
    }

    pt_result_t result;
    result.color = f4_f3(light);
    result.albedo = f4_f3(albedo);
    result.normal = f4_f3(normal);
    return result;
}

static void TraceFn(task_t* pbase, i32 begin, i32 end)
{
    trace_task_t* task = (trace_task_t*)pbase;

    const pt_scene_t* scene = task->trace.scene;

    float3* pim_noalias color = task->trace.color;
    float3* pim_noalias albedo = task->trace.albedo;
    float3* pim_noalias normal = task->trace.normal;

    const int2 size = task->trace.imageSize;
    const float sampleWeight = task->trace.sampleWeight;
    const i32 bounces = task->trace.bounces;
    const camera_t camera = task->camera;
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
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
        normal[i] = f3_normalize(normal[i]);
    }

    prng_set(rng);
}

ProfileMark(pm_trace, pt_trace)
void pt_trace(pt_trace_t* desc)
{
    ProfileBegin(pm_trace);

    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(desc->camera);
    ASSERT(desc->color);
    ASSERT(desc->albedo);
    ASSERT(desc->normal);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->trace = *desc;
    task->camera = desc->camera[0];

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_run(&task->task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

static void RayGenFn(task_t* pBase, i32 begin, i32 end)
{
    prng_t rng = prng_get();

    pt_raygen_t* task = (pt_raygen_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const pt_dist_t dist = task->dist;
    const i32 bounces = task->bounces;
    const ray_t origin = task->origin;
    const float3x3 TBN = NormalToTBN(origin.rd);

    float4* pim_noalias colors = task->colors;
    float4* pim_noalias directions = task->directions;

    ray_t ray;
    ray.ro = origin.ro;

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
        colors[i] = f3_f4(result.color, 1.0f);
    }

    prng_set(rng);
}

ProfileMark(pm_raygen, pt_raygen)
pt_results_t pt_raygen(
    const pt_scene_t* scene,
    ray_t origin,
    pt_dist_t dist,
    i32 count,
    i32 bounces)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    pt_raygen_t* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = tmp_malloc(sizeof(task->colors[0]) * count);
    task->directions = tmp_malloc(sizeof(task->directions[0]) * count);
    task->dist = dist;
    task->bounces = bounces;
    task_run(&task->task, RayGenFn, count);

    pt_results_t results =
    {
        .colors = task->colors,
        .directions = task->directions,
    };

    ProfileEnd(pm_raygen);

    return results;
}
