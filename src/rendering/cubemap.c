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

static Cubemaps ms_cubemaps;
Cubemaps* Cubemaps_Get(void) { return &ms_cubemaps; }

i32 Cubemaps_Add(Cubemaps* maps, Guid name, i32 size, Box3D bounds)
{
    ASSERT(maps);
    ASSERT(!guid_isnull(name));
    ASSERT(guid_find(maps->names, maps->count, name) == -1);

    const i32 back = maps->count++;
    const i32 len = back + 1;

    Perm_Reserve(maps->names, len);
    Perm_Reserve(maps->bounds, len);
    Perm_Reserve(maps->cubemaps, len);

    maps->names[back] = name;
    maps->bounds[back] = bounds;
    Cubemap_New(maps->cubemaps + back, size);

    return back;
}

bool Cubemaps_Rm(Cubemaps* maps, Guid name)
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

i32 Cubemaps_Find(const Cubemaps* maps, Guid name)
{
    if (guid_isnull(name))
    {
        return -1;
    }
    return guid_find(maps->names, maps->count, name);
}

void Cubemap_New(Cubemap* cm, i32 size)
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
        cm->color[i] = Tex_Calloc(sizeof(cm->color[0][0]) * len);
        cm->convolved[i] = Tex_Calloc(sizeof(cm->convolved[0][0]) * elemCount);
    }
}

void Cubemap_Del(Cubemap* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            Mem_Free(cm->color[i]);
            Mem_Free(cm->convolved[i]);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

typedef struct cmbake_s
{
    Task task;
    Cubemap* cm;
    PtScene* scene;
    float4 origin;
    float weight;
} cmbake_t;

static void BakeFn(Task* pBase, i32 begin, i32 end)
{
    cmbake_t* task = (cmbake_t*)pBase;

    Cubemap* cm = task->cm;
    PtScene* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;

    const i32 size = cm->size;
    const i32 flen = size * size;

    PtSampler sampler = PtSampler_Get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / flen;
        i32 fi = i % flen;
        int2 coord = { fi % size, fi / size };
        float2 Xi = f2_tent(Pt_Sample2D(&sampler));
        float4 dir = Cubemap_CalcDir(size, face, coord, Xi);
        PtResult result = Pt_TraceRay(&sampler, scene, origin, dir);
        cm->color[face][fi] = f3_lerp(cm->color[face][fi], result.color, weight);
    }
    PtSampler_Set(sampler);
}

ProfileMark(pm_Bake, Cubemap_Bake)
void Cubemap_Bake(
    Cubemap* cm,
    PtScene* scene,
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

        cmbake_t* task = Temp_Calloc(sizeof(*task));
        task->cm = cm;
        task->scene = scene;
        task->origin = origin;
        task->weight = weight;

        Task_Run(&task->task, BakeFn, size * size * Cubeface_COUNT);

        ProfileEnd(pm_Bake);
    }
}

static float4 VEC_CALL PrefilterEnvMap(
    Prng* rng,
    const Cubemap* cm,
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
    Task task;
    Cubemap* cm;
    i32 mip;
    i32 size;
    u32 sampleCount;
    float weight;
} prefilter_t;

static void PrefilterFn(Task* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    Cubemap* cm = task->cm;

    const u32 sampleCount = task->sampleCount;
    const float weight = task->weight;
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = MipToRoughness((float)mip);

    Prng rng = Prng_Get();
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
    Prng_Set(rng);
}

ProfileMark(pm_Convolve, Cubemap_Convolve)
void Cubemap_Convolve(
    Cubemap* cm,
    u32 sampleCount,
    float weight)
{
    ASSERT(cm);

    ProfileBegin(pm_Convolve);

    const i32 mipCount = cm->mipCount;
    const i32 size = cm->size;

    i32 numSubmit = 0;
    prefilter_t* tasks = Temp_Calloc(sizeof(tasks[0]) * mipCount);
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
            Task_Submit(&tasks[m].task, PrefilterFn, len);
            ++numSubmit;
        }
    }

    TaskSys_Schedule();

    for (i32 m = 0; m < numSubmit; ++m)
    {
        Task_Await(&tasks[m].task);
    }

    ProfileEnd(pm_Convolve);
}
