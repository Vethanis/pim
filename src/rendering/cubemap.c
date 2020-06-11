#include "rendering/cubemap.h"
#include "allocator/allocator.h"
#include "rendering/sampler.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/quat_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "rendering/denoise.h"
#include "math/sampling.h"
#include "math/frustum.h"
#include "common/profiler.h"
#include "common/find.h"
#include <string.h>

static const float4 kForwards[] =
{
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, -1.0f},
};

static const float4 kUps[] =
{
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
};

static const float4 kRights[] =
{
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
};

static Cubemaps_t ms_cubemaps;

Cubemaps_t* Cubemaps_Get(void) { return &ms_cubemaps; }

i32 Cubemaps_Add(u32 name, i32 size, sphere_t bounds)
{
    const i32 back = ms_cubemaps.count;
    const i32 len = back + 1;
    ms_cubemaps.count = len;

    PermGrow(ms_cubemaps.names, len);
    PermGrow(ms_cubemaps.bounds, len);
    PermGrow(ms_cubemaps.cubemaps, len);

    ms_cubemaps.names[back] = name;
    ms_cubemaps.bounds[back] = bounds;
    Cubemap_New(ms_cubemaps.cubemaps + back, size);

    return back;
}

bool Cubemaps_Rm(u32 name)
{
    const i32 i = Cubemaps_Find(name);
    if (i == -1)
    {
        return false;
    }

    Cubemap_Del(ms_cubemaps.cubemaps + i);
    const i32 len = ms_cubemaps.count;
    ms_cubemaps.count = len - 1;

    PopSwap(ms_cubemaps.names, i, len);
    PopSwap(ms_cubemaps.bounds, i, len);
    PopSwap(ms_cubemaps.cubemaps, i, len);

    return true;
}

i32 Cubemaps_Find(u32 name)
{
    return Find_u32(ms_cubemaps.names, ms_cubemaps.count, name);
}

void Cubemap_New(Cubemap* cm, i32 size)
{
    ASSERT(cm);
    ASSERT(size >= 0);

    const i32 len = size * size;
    const int2 s = { size, size };
    const i32 mipCount = CalcMipCount(s);
    const i32 elemCount = CalcMipOffset(s, mipCount) + 1;

    cm->size = size;
    cm->mipCount = mipCount;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        cm->color[i] = perm_calloc(sizeof(cm->color[0][0]) * len);
        cm->albedo[i] = perm_calloc(sizeof(cm->albedo[0][0]) * len);
        cm->normal[i] = perm_calloc(sizeof(cm->normal[0][0]) * len);
        cm->denoised[i] = perm_calloc(sizeof(cm->denoised[0][0]) * len);
        cm->convolved[i] = perm_calloc(sizeof(cm->convolved[0][0]) * elemCount);
    }
}

void Cubemap_Del(Cubemap* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            pim_free(cm->color[i]);
            pim_free(cm->albedo[i]);
            pim_free(cm->normal[i]);
            pim_free(cm->denoised[i]);
            pim_free(cm->convolved[i]);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

pim_optimize
Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut)
{
    ASSERT(uvOut);

    float4 absdir = f4_abs(dir);
    float v = f4_hmax3(absdir);
    float ma = 0.5f / v;

    Cubeface face;
    if (v == absdir.x)
    {
        face = dir.x < 0.0f ? Cubeface_XM : Cubeface_XP;
    }
    else if (v == absdir.y)
    {
        face = dir.y < 0.0f ? Cubeface_YM : Cubeface_YP;
    }
    else
    {
        face = dir.z < 0.0f ? Cubeface_ZM : Cubeface_ZP;
    }

    float2 uv;
    uv.x = f4_dot3(kRights[face], dir);
    uv.y = f4_dot3(kUps[face], dir);
    uv = f2_addvs(f2_mulvs(uv, ma), 0.5f);

    *uvOut = uv;
    return face;
}

pim_optimize
float4 VEC_CALL Cubemap_ReadConvolved(const Cubemap* cm, float4 dir, float mip)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    mip = f1_clamp(mip, 0.0f, (float)(cm->mipCount - 1));
    int2 size = { cm->size, cm->size };
    const float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    return TrilinearClamp_f4(buffer, size, uv, mip);
}

pim_optimize
float3 VEC_CALL Cubemap_ReadColor(const Cubemap* cm, float4 dir)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    int2 size = { cm->size, cm->size };
    const float3* pim_noalias buffer = cm->color[face];
    ASSERT(buffer);

    return UvBilinearClamp_f3(buffer, size, uv);
}

pim_optimize
float3 VEC_CALL Cubemap_ReadAlbedo(const Cubemap* cm, float4 dir)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    int2 size = { cm->size, cm->size };
    const float3* pim_noalias buffer = cm->albedo[face];
    ASSERT(buffer);

    return UvBilinearClamp_f3(buffer, size, uv);
}

pim_optimize
float3 VEC_CALL Cubemap_ReadNormal(const Cubemap* cm, float4 dir)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    int2 size = { cm->size, cm->size };
    const float3* pim_noalias buffer = cm->normal[face];
    ASSERT(buffer);

    return UvBilinearClamp_f3(buffer, size, uv);
}


pim_optimize
float3 VEC_CALL Cubemap_ReadDenoised(const Cubemap* cm, float4 dir)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    int2 size = { cm->size, cm->size };
    const float3* pim_noalias buffer = cm->denoised[face];
    ASSERT(buffer);

    return UvBilinearClamp_f3(buffer, size, uv);
}

pim_optimize
void VEC_CALL Cubemap_Write(Cubemap* cm, Cubeface face, int2 coord, float4 value)
{
    ASSERT(cm);
    float4* buffer = cm->convolved[face];
    ASSERT(buffer);
    Write_f4(buffer, i2_s(cm->size), coord, value);
}

pim_optimize
void VEC_CALL Cubemap_WriteMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value)
{
    ASSERT(cm);
    float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    int2 size = i2_s(cm->size);
    i32 offset = CalcMipOffset(size, mip);
    buffer += offset;

    int2 mipSize = CalcMipSize(size, mip);
    i32 i = Clamp(mipSize, coord);
    buffer[i] = value;
}

pim_optimize
static void VEC_CALL Cubemap_BlendMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value,
    float weight)
{
    ASSERT(cm);
    float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    int2 size = i2_s(cm->size);
    i32 offset = CalcMipOffset(size, mip);
    buffer += offset;

    int2 mipSize = CalcMipSize(size, mip);
    i32 i = Clamp(mipSize, coord);
    buffer[i] = f4_lerpvs(buffer[i], value, weight);
}

pim_optimize
float4 VEC_CALL Cubemap_CalcDir(
    i32 size, Cubeface face, int2 coord, float2 Xi)
{
    float2 uv = CoordToUv(i2_s(size), coord);
    Xi = f2_mulvs(Xi, 1.0f / size);
    uv = f2_add(uv, Xi);
    uv = f2_snorm(uv);

    float4 fwd = kForwards[face];
    float4 right = kRights[face];
    float4 up = kUps[face];
    float4 dir = proj_dir(right, up, fwd, f2_1, uv);

    return dir;
}

typedef struct cmbake_s
{
    task_t;
    Cubemap* cm;
    const pt_scene_t* scene;
    float4 origin;
    float weight;
    i32 bounces;
} cmbake_t;

pim_optimize
static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    cmbake_t* task = (cmbake_t*)pBase;

    Cubemap* cm = task->cm;
    const pt_scene_t* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;
    const i32 bounces = task->bounces;

    const i32 size = cm->size;
    const i32 flen = size * size;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / flen;
        i32 fi = i % flen;
        int2 coord = { fi % size, fi / size };
        float4 dir = Cubemap_CalcDir(size, face, coord, f2_tent(f2_rand(&rng)));
        ray_t ray = { origin, dir };
        pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);
        cm->color[face][fi] = f3_lerp(cm->color[face][fi], result.color, weight);
        cm->albedo[face][fi] = f3_lerp(cm->albedo[face][fi], result.albedo, weight);
        cm->normal[face][fi] = f3_lerp(cm->normal[face][fi], result.normal, weight);
    }
    prng_set(rng);
}

ProfileMark(pm_Bake, Cubemap_Bake)
void Cubemap_Bake(
    Cubemap* cm,
    const pt_scene_t* scene,
    float4 origin,
    float weight,
    i32 bounces)
{
    ASSERT(cm);
    ASSERT(scene);
    ASSERT(weight > 0.0f);
    ASSERT(bounces >= 0);

    i32 size = cm->size;
    if (size > 0)
    {
        ProfileBegin(pm_Bake);

        cmbake_t* task = tmp_calloc(sizeof(*task));
        task->cm = cm;
        task->scene = scene;
        task->origin = origin;
        task->weight = weight;
        task->bounces = bounces;

        task_run((task_t*)task, BakeFn, size * size * Cubeface_COUNT);

        ProfileEnd(pm_Bake);
    }
}

pim_optimize
static float4 VEC_CALL PrefilterEnvMap(
    const Cubemap* cm,
    float3x3 TBN,
    float4 N,
    u32 sampleCount,
    float roughness)
{
    // N=V approximation
    float4 I = f4_neg(N);

    float weight = 0.0f;
    float4 light = f4_0;
    prng_t rng = prng_get();
    for (u32 i = 0; i < sampleCount; ++i)
    {
        //float2 Xi = Hammersley2D(i, sampleCount);
        float2 Xi = f2_rand(&rng);
        float4 H = TbnToWorld(TBN, SampleGGXMicrofacet(Xi, roughness));
        float4 L = f4_normalize3(f4_reflect3(I, H));
        float NoL = f4_dot3(L, N);
        if (NoL > 0.0f)
        {
            float4 sample = f3_f4(Cubemap_ReadDenoised(cm, L), 1.0f);
            sample = f4_mulvs(sample, NoL);
            light = f4_add(light, sample);
            weight += NoL;
        }
    }
    prng_set(rng);
    if (weight > 0.0f)
    {
        light = f4_divvs(light, weight);
    }
    return light;
}

typedef struct prefilter_s
{
    task_t task;
    Cubemap* cm;
    i32 mip;
    i32 size;
    u32 sampleCount;
    float weight;
} prefilter_t;

pim_optimize
static void PrefilterFn(task_t* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    Cubemap* cm = task->cm;

    const u32 sampleCount = task->sampleCount;
    const float weight = task->weight;
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = Cubemap_Mip2Rough((float)mip);

    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        i32 fi = i % len;
        ASSERT(face < Cubeface_COUNT);
        int2 coord = { fi % size, fi / size };

        float4 N = Cubemap_CalcDir(size, face, coord, f2_0);
        float3x3 TBN = NormalToTBN(N);

        float4 light = PrefilterEnvMap(cm, TBN, N, sampleCount, roughness);
        Cubemap_BlendMip(cm, face, coord, mip, light, weight);
    }
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

    task_t** tasks = tmp_calloc(sizeof(tasks[0]) * mipCount);
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * Cubeface_COUNT;

        if (len > 0)
        {
            prefilter_t* task = tmp_calloc(sizeof(*task));
            task->cm = cm;
            task->mip = m;
            task->size = mSize;
            task->sampleCount = sampleCount;
            task->weight = weight;
            tasks[m] = (task_t*)task;
            task_submit((task_t*)task, PrefilterFn, len);
        }
    }

    task_sys_schedule();

    for (i32 m = 0; m < mipCount; ++m)
    {
        if (tasks[m])
        {
            task_await(tasks[m]);
        }
    }

    ProfileEnd(pm_Convolve);
}

ProfileMark(pm_Denoise, Cubemap_Denoise)
void Cubemap_Denoise(Cubemap* cm)
{
    ASSERT(cm);

    ProfileBegin(pm_Denoise);

    int2 size = { cm->size, cm->size };
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        float3* color = cm->color[i];
        float3* albedo = cm->albedo[i];
        float3* normal = cm->normal[i];
        float3* denoised = cm->denoised[i];
        ASSERT(color);
        ASSERT(albedo);
        ASSERT(normal);
        ASSERT(denoised);
        Denoise(DenoiseType_Image, size, color, albedo, normal, denoised);
    }

    ProfileEnd(pm_Denoise);
}
