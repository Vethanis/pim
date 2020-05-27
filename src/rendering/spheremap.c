#include "rendering/spheremap.h"
#include "allocator/allocator.h"
#include "rendering/sampler.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "common/random.h"
#include "math/float3_funcs.h"

void SphereMap_New(SphereMap* map, int2 size)
{
    ASSERT(map);
    i32 len = size.x * size.y;
    ASSERT(len >= 0);
    map->texels = perm_calloc(sizeof(map->texels[0]) * len);
    map->size = size;
}

void SphereMap_Del(SphereMap* map)
{
    if (map)
    {
        pim_free(map->texels);
        map->texels = NULL;
        map->size.x = 0;
        map->size.y = 0;
    }
}

float2 VEC_CALL SphereMap_DirToUv(float4 dir)
{
    float phi = atan2f(dir.y, dir.x);
    float z = dir.z;
    float u = 0.5f * z + 0.5f;
    float v = phi / kTau;
    v = v < 0.0f ? v + 1.0f : v;
    float2 Xi = { u, v };
    return Xi;
}

float4 VEC_CALL SphereMap_UvToDir(float2 Xi)
{
    // discontinuities occur around 0 and tau
    const float e = 0.00001f;
    Xi = f2_clamp(Xi, f2_s(e), f2_s(1.0f - e));
    float z = Xi.x * 2.0f - 1.0f;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float4 dir = { x, y, z, 0.0f };
    return dir;
}

float4 VEC_CALL SphereMap_Read(const SphereMap* map, float4 dir)
{
    float2 uv = SphereMap_DirToUv(dir);
    return UvBilinearClamp_f4(map->texels, map->size, uv);
}

void VEC_CALL SphereMap_Write(SphereMap* map, float4 dir, float4 color)
{
    float2 uv = SphereMap_DirToUv(dir);
    i32 offset = UvClamp(map->size, uv);
    map->texels[offset] = color;
}

void VEC_CALL SphereMap_Fit(SphereMap* map, float4 dir, float4 color, float weight)
{
    ASSERT(map);
    float4* pim_noalias texels = map->texels;
    ASSERT(texels);
    float2 uv = SphereMap_DirToUv(dir);
    i32 offset = UvClamp(map->size, uv);
    texels[offset] = f4_lerpvs(texels[offset], color, weight);
}

typedef struct smbake_s
{
    task_t task;
    const pt_scene_t* scene;
    int2 size;
    float3* colors;
    float3* albedos;
    float3* normals;
    float4 origin;
    float weight;
    i32 bounces;
} smbake_t;

static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    smbake_t* task = (smbake_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;
    const i32 bounces = task->bounces;
    const int2 size = task->size;
    float3* pim_noalias colors = task->colors;
    float3* pim_noalias albedos = task->albedos;
    float3* pim_noalias normals = task->normals;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        const i32 x = i % size.x;
        const i32 y = i / size.x;
        float2 jit = f2_tent(f2_rand(&rng));
        jit = f2_mulvs(jit, 0.5f);
        jit = f2_div(jit, f2_iv(x, y));
        float2 uv = { (float)x / size.x, (float)y / size.y };
        uv = f2_add(uv, jit);
        const float4 dir = SphereMap_UvToDir(uv);
        ray_t ray = { .ro = origin, .rd = dir };
        pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);
        colors[i] = f3_lerp(colors[i], result.color, weight);
        albedos[i] = f3_lerp(albedos[i], result.albedo, weight);
        normals[i] = f3_lerp(normals[i], result.normal, weight);
    }
    prng_set(rng);
}

struct task_s* SphereMap_Bake(
    const struct pt_scene_s* scene,
    int2 size,
    float3* colors,
    float3* albedos,
    float3* normals,
    float4 origin,
    float weight,
    i32 bounces)
{
    ASSERT(scene);
    ASSERT(map);
    ASSERT(map->texels);
    ASSERT(weight >= 0.0f);
    ASSERT(bounces >= 0);

    const i32 len = size.x * size.y;
    ASSERT(len >= 0);

    smbake_t* task = tmp_calloc(sizeof(*task));
    task->bounces = bounces;
    task->size = size;
    task->colors = colors;
    task->albedos = albedos;
    task->normals = normals;
    task->origin = origin;
    task->scene = scene;
    task->weight = weight;
    task_submit((task_t*)task, BakeFn, len);

    return (task_t*)task;
}
