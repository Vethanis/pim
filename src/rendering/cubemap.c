#include "rendering/cubemap.h"
#include "allocator/allocator.h"
#include "rendering/sampler.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/quat_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "math/sampling.h"
#include "math/frustum.h"
#include "rendering/denoise.h"
#include "common/profiler.h"
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

void Cubemap_New(Cubemap* cm, i32 size)
{
    ASSERT(cm);

    const int2 s = { size, size };
    const i32 mipCount = CalcMipCount(s);
    const i32 elemCount = CalcMipOffset(s, mipCount) + 1;

    cm->size = size;
    cm->mipCount = mipCount;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        cm->faces[i] = perm_calloc(sizeof(cm->faces[0][0]) * elemCount);
    }
}

void Cubemap_Del(Cubemap* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            pim_free(cm->faces[i]);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

void BCubemap_New(BCubemap* cm, i32 size)
{
    ASSERT(cm);
    ASSERT(size >= 0);

    cm->size = size;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        trace_img_t img = { 0 };
        trace_img_new(&img, i2_s(size));
        cm->faces[i] = img;
    }
}

void BCubemap_Del(BCubemap* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            trace_img_del(cm->faces + i);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

void Cubemap_Cpy(const Cubemap* src, Cubemap* dst)
{
    ASSERT(src);
    ASSERT(dst);
    ASSERT(src->size == dst->size);
    i32 len = CalcMipOffset(i2_s(src->size), src->mipCount) + 1;
    for (i32 f = 0; f < Cubeface_COUNT; ++f)
    {
        const float4* pim_noalias srcTexels = src->faces[f];
        float4* pim_noalias dstTexels = dst->faces[f];
        for (i32 i = 0; i < len; ++i)
        {
            dstTexels[i] = srcTexels[i];
        }
    }
}

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

float4 VEC_CALL Cubemap_Read(const Cubemap* cm, float4 dir, float mip)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    mip = f1_clamp(mip, 0.0f, (float)(cm->mipCount - 1));
    int2 size = { cm->size, cm->size };
    const float4* buffer = cm->faces[face];
    ASSERT(buffer);

    return TrilinearClamp_f4(buffer, size, uv, mip);
}

void VEC_CALL Cubemap_Write(Cubemap* cm, Cubeface face, int2 coord, float4 value)
{
    ASSERT(cm);
    float4* buffer = cm->faces[face];
    ASSERT(buffer);
    Write_f4(buffer, i2_s(cm->size), coord, value);
}

void VEC_CALL Cubemap_WriteMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value)
{
    ASSERT(cm);
    float4* buffer = cm->faces[face];
    ASSERT(buffer);

    int2 size = i2_s(cm->size);
    i32 offset = CalcMipOffset(size, mip);
    buffer += offset;

    int2 mipSize = CalcMipSize(size, mip);

    Write_f4(buffer, mipSize, coord, value);
}

float4 VEC_CALL Cubemap_CalcDir(
    i32 size, Cubeface face, int2 coord, float2 Xi)
{
    Xi = f2_mulvs(Xi, 1.0f / size);
    float2 uv = CoordToUv(i2_s(size), coord);
    uv = f2_add(uv, Xi);
    uv = f2_snorm(uv);

    float4 fwd = kForwards[face];
    float4 right = kRights[face];
    float4 up = kUps[face];
    float4 dir = proj_dir(right, up, fwd, f2_1, uv);

    return dir;
}

float4 VEC_CALL Cubemap_FaceDir(Cubeface face)
{
    return kForwards[face];
}

void Cubemap_GenMips(Cubemap* cm)
{
    ASSERT(cm);

    const i32 mipCount = cm->mipCount;
    const int2 size = i2_s(cm->size);

    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        float4* buffer = cm->faces[i];
        ASSERT(buffer);

        for (i32 m = 1; m < mipCount; ++m)
        {
            const i32 n = m - 1;

            const int2 mSize = CalcMipSize(size, m);
            const int2 nSize = CalcMipSize(size, n);

            const i32 mOffset = CalcMipOffset(size, m);
            const i32 nOffset = CalcMipOffset(size, n);

            float4* pim_noalias mBuffer = buffer + mOffset;
            const float4* pim_noalias nBuffer = buffer + nOffset;

            for (i32 y = 0; y < mSize.y; ++y)
            {
                for (i32 x = 0; x < mSize.x; ++x)
                {
                    int2 mCoord = i2_v(x, y);
                    int2 nCoord = i2_mulvs(mCoord, 2);
                    float4 nAvg = MipAvg_f4(nBuffer, nSize, nCoord);
                    Write_f4(mBuffer, mSize, mCoord, nAvg);
                }
            }
        }
    }
}

typedef struct cmbake_s
{
    task_t;
    BCubemap* bakemap;
    const pt_scene_t* scene;
    float4 origin;
    float weight;
    i32 bounces;
} cmbake_t;

static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    cmbake_t* task = (cmbake_t*)pBase;

    BCubemap* bakemap = task->bakemap;
    const pt_scene_t* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;
    const i32 bounces = task->bounces;

    const i32 size = bakemap->size;
    const i32 flen = size * size;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / flen;
        i32 fi = i % flen;
        int2 coord = { fi % size, fi / size };
        float4 dir = Cubemap_CalcDir(size, face, coord, f2_tent(f2_rand(&rng)));
        ray_t ray = { .ro = origin,.rd = dir };
        pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);
        trace_img_t img = bakemap->faces[face];
        img.colors[fi] = f3_lerp(img.colors[fi], result.color, weight);
        img.albedos[fi] = f3_lerp(img.albedos[fi], result.albedo, weight);
        img.normals[fi] = f3_lerp(img.normals[fi], result.normal, weight);
    }
    prng_set(rng);
}

ProfileMark(pm_Bake, Cubemap_Bake)
task_t* Cubemap_Bake(
    BCubemap* bakemap,
    const pt_scene_t* scene,
    float4 origin,
    float weight,
    i32 bounces)
{
    ASSERT(bakemap);
    ASSERT(scene);
    ASSERT(weight > 0.0f);
    ASSERT(bounces >= 0);

    i32 size = bakemap->size;
    if (size > 0)
    {
        ProfileBegin(pm_Bake);

        cmbake_t* task = tmp_calloc(sizeof(*task));
        task->bakemap = bakemap;
        task->scene = scene;
        task->origin = origin;
        task->weight = weight;
        task->bounces = bounces;

        task_submit((task_t*)task, BakeFn, size * size * 6);

        ProfileEnd(pm_Bake);
        return (task_t*)task;
    }

    return NULL;
}

static denoise_t ms_denoise;
ProfileMark(pm_Denoise, Cubemap_Denoise)
void Cubemap_Denoise(BCubemap* src, Cubemap* dst)
{
    ASSERT(src);
    ASSERT(dst);

    ProfileBegin(pm_Denoise);

    if (!ms_denoise.device)
    {
        Denoise_New(&ms_denoise);
    }

    i32 size = dst->size;
    i32 len = size * size;

    float3* pim_noalias output = tmp_malloc(sizeof(output[0]) * len);
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        Denoise_Execute(&ms_denoise, DenoiseType_Image, src->faces[i], output);

        float4* pim_noalias texels = dst->faces[i];
        for (i32 j = 0; j < len; ++j)
        {
            texels[j] = f3_f4(output[j], 1.0f);
        }
    }

    ProfileEnd(pm_Denoise);
}

static float4 VEC_CALL PrefilterEnvMap(
    const Cubemap* cm,
    float3x3 TBN,
    float4 N,
    u32 sampleCount,
    float roughness)
{
    // N=V approximation

    float weight = 0.0f;
    float4 light = f4_0;
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float2 Xi = Hammersley2D(i, sampleCount);
        float4 H = TbnToWorld(TBN, SampleGGXMicrofacet(Xi, roughness));
        float4 L = f4_normalize3(f4_reflect3(f4_neg(N), H));
        float NoL = f4_dot3(L, N);
        if (NoL > 0.0f)
        {
            float4 sample = Cubemap_Read(cm, L, 0.0f);
            sample = f4_mulvs(sample, NoL);
            light = f4_add(light, sample);
            weight += NoL;
        }
    }
    if (weight > 0.0f)
    {
        light = f4_divvs(light, weight);
    }
    return light;
}

typedef struct prefilter_s
{
    task_t task;
    const Cubemap* src;
    Cubemap* dst;
    i32 mip;
    i32 size;
    u32 sampleCount;
} prefilter_t;

static void PrefilterFn(task_t* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    const Cubemap* pim_noalias src = task->src;
    Cubemap* pim_noalias dst = task->dst;

    const u32 sampleCount = task->sampleCount;
    const i32 mipCount = i1_min(src->mipCount, dst->mipCount);
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = Cubemap_Mip2Rough((float)mip);

    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        ASSERT(face < Cubeface_COUNT);
        i32 fi = i % len;
        int2 coord = { fi % size, fi / size };

        float4 N = Cubemap_CalcDir(size, face, coord, f2_0);
        float3x3 TBN = NormalToTBN(N);

        float4 light = PrefilterEnvMap(src, TBN, N, sampleCount, roughness);
        Cubemap_WriteMip(dst, face, coord, mip, light);
    }
}

ProfileMark(pm_Prefilter, Cubemap_Prefilter)
void Cubemap_Prefilter(const Cubemap* src, Cubemap* dst, u32 sampleCount)
{
    ASSERT(src);
    ASSERT(dst);

    ProfileBegin(pm_Prefilter);

    const i32 mipCount = i1_min(7, i1_min(src->mipCount, dst->mipCount));
    const i32 size = i1_min(src->size, dst->size);

    task_t** tasks = tmp_calloc(sizeof(tasks[0]) * mipCount);
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * Cubeface_COUNT;

        if (len > 0)
        {
            prefilter_t* task = tmp_calloc(sizeof(*task));
            task->src = src;
            task->dst = dst;
            task->mip = m;
            task->size = mSize;
            task->sampleCount = sampleCount;
            task_submit((task_t*)task, PrefilterFn, len);
            tasks[m] = (task_t*)task;
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

    ProfileEnd(pm_Prefilter);
}
