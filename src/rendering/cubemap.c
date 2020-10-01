#include "rendering/cubemap.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/quat_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "rendering/denoise.h"
#include "math/sampling.h"
#include "common/profiler.h"
#include <string.h>

static cubemaps_t ms_cubemaps;
cubemaps_t* Cubemaps_Get(void) { return &ms_cubemaps; }

i32 Cubemaps_Add(cubemaps_t* maps, guid_t name, i32 size, box_t bounds)
{
    ASSERT(maps);
    ASSERT(!guid_isnull(name));
    ASSERT(guid_find(maps->names, maps->count, name) == -1);

    const i32 back = maps->count++;
    const i32 len = back + 1;

    PermReserve(maps->names, len);
    PermReserve(maps->bounds, len);
    PermReserve(maps->cubemaps, len);

    maps->names[back] = name;
    maps->bounds[back] = bounds;
    Cubemap_New(maps->cubemaps + back, size);

    return back;
}

bool Cubemaps_Rm(cubemaps_t* maps, guid_t name)
{
    ASSERT(maps);

    i32 i = Cubemaps_Find(maps, name);
    if (i == -1)
    {
        return false;
    }

    Cubemap_Del(maps->cubemaps + i);
    i32 len = maps->count--;

    PopSwap(maps->names, i, len);
    PopSwap(maps->bounds, i, len);
    PopSwap(maps->cubemaps, i, len);

    return true;
}

i32 Cubemaps_Find(const cubemaps_t* maps, guid_t name)
{
    if (guid_isnull(name))
    {
        return -1;
    }
    return guid_find(maps->names, maps->count, name);
}

void Cubemap_New(cubemap_t* cm, i32 size)
{
    ASSERT(cm);
    ASSERT(size >= 0);

    memset(cm, 0, sizeof(*cm));

    const i32 len = size * size;
    const int2 s = { size, size };
    const i32 mipCount = CalcMipCount(s);
    const i32 elemCount = CalcMipOffset(s, mipCount) + 1;

    cm->size = size;
    cm->mipCount = mipCount;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        cm->color[i] = tex_calloc(sizeof(cm->color[0][0]) * len);
        cm->convolved[i] = tex_calloc(sizeof(cm->convolved[0][0]) * elemCount);
    }
}

void Cubemap_Del(cubemap_t* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            pim_free(cm->color[i]);
            pim_free(cm->convolved[i]);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

typedef struct cmbake_s
{
    task_t task;
    cubemap_t* cm;
    pt_scene_t* scene;
    float4 origin;
    float weight;
} cmbake_t;

static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    cmbake_t* task = (cmbake_t*)pBase;

    cubemap_t* cm = task->cm;
    pt_scene_t* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;

    const i32 size = cm->size;
    const i32 flen = size * size;

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / flen;
        i32 fi = i % flen;
        int2 coord = { fi % size, fi / size };
        float2 Xi = f2_tent(pt_sample_2d(&sampler));
        float4 dir = Cubemap_CalcDir(size, face, coord, Xi);
        ray_t ray = { origin, dir };
        pt_result_t result = pt_trace_ray(&sampler, scene, ray);
        cm->color[face][fi] = f3_lerp(cm->color[face][fi], result.color, weight);
    }
    pt_sampler_set(sampler);
}

ProfileMark(pm_Bake, Cubemap_Bake)
void Cubemap_Bake(
    cubemap_t* cm,
    pt_scene_t* scene,
    float4 origin,
    float weight)
{
    ASSERT(cm);
    ASSERT(scene);
    ASSERT(weight > 0.0f);

    i32 size = cm->size;
    if (size > 0)
    {
        ProfileBegin(pm_Bake);

        cmbake_t* task = tmp_calloc(sizeof(*task));
        task->cm = cm;
        task->scene = scene;
        task->origin = origin;
        task->weight = weight;

        task_run(&task->task, BakeFn, size * size * Cubeface_COUNT);

        ProfileEnd(pm_Bake);
    }
}

static float4 VEC_CALL PrefilterEnvMap(
    prng_t* rng,
    const cubemap_t* cm,
    float3x3 TBN,
    u32 sampleCount,
    float roughness)
{
    // split sum N=V approximation
    float4 I = f4_neg(TBN.c2);
    float alpha = BrdfAlpha(roughness);

    float basisIntegral = 0.0f;
    float4 lightIntegral = f4_0;
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float4 H = TbnToWorld(TBN, SampleGGXMicrofacet(f2_rand(rng), alpha));
        float4 L = f4_reflect3(I, H);
        float NoL = f4_dot3(L, TBN.c2);
        if (NoL > 0.0f)
        {
            float4 sample = f3_f4(Cubemap_ReadColor(cm, L), 1.0f);
            sample = f4_mulvs(sample, NoL);
            lightIntegral = f4_add(lightIntegral, sample);
            basisIntegral += NoL;
        }
    }
    if (basisIntegral > 0.0f)
    {
        lightIntegral = f4_divvs(lightIntegral, basisIntegral);
    }
    return lightIntegral;
}

typedef struct prefilter_s
{
    task_t task;
    cubemap_t* cm;
    i32 mip;
    i32 size;
    u32 sampleCount;
    float weight;
} prefilter_t;

static void PrefilterFn(task_t* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    cubemap_t* cm = task->cm;

    const u32 sampleCount = task->sampleCount;
    const float weight = task->weight;
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = MipToRoughness((float)mip);

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        i32 fi = i % len;
        ASSERT(face < Cubeface_COUNT);
        int2 coord = { fi % size, fi / size };

        float2 Xi = f2_tent(f2_rand(&rng));
        float4 N = Cubemap_CalcDir(size, face, coord, Xi);
        float3x3 TBN = NormalToTBN(N);

        float4 light = PrefilterEnvMap(&rng, cm, TBN, sampleCount, roughness);
        Cubemap_BlendMip(cm, face, coord, mip, light, weight);
    }
    prng_set(rng);
}

ProfileMark(pm_Convolve, Cubemap_Convolve)
void Cubemap_Convolve(
    cubemap_t* cm,
    u32 sampleCount,
    float weight)
{
    ASSERT(cm);

    ProfileBegin(pm_Convolve);

    const i32 mipCount = cm->mipCount;
    const i32 size = cm->size;

    i32 numSubmit = 0;
    prefilter_t* tasks = tmp_calloc(sizeof(tasks[0]) * mipCount);
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * Cubeface_COUNT;
        if (len > 0)
        {
            tasks[m].cm = cm;
            tasks[m].mip = m;
            tasks[m].size = mSize;
            tasks[m].sampleCount = sampleCount;
            tasks[m].weight = weight;
            task_submit(&tasks[m].task, PrefilterFn, len);
            ++numSubmit;
        }
    }

    task_sys_schedule();

    for (i32 m = 0; m < numSubmit; ++m)
    {
        task_await(&tasks[m].task);
    }

    ProfileEnd(pm_Convolve);
}
