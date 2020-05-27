#include "rendering/cubemap.h"
#include "allocator/allocator.h"
#include "rendering/sampler.h"
#include "math/float4_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "math/sampling.h"
#include <string.h>

Cubemap* Cubemap_New(i32 size)
{
    const int2 s = { size, size };
    const i32 mipCount = CalcMipCount(s);
    const i32 elemCount = CalcMipOffset(s, mipCount) + 1;

    Cubemap* cm = perm_calloc(sizeof(*cm));
    cm->size = size;
    cm->mipCount = mipCount;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        cm->faces[i] = perm_calloc(sizeof(cm->faces[0][0]) * elemCount);
    }
    return cm;
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
        pim_free(cm);
    }
}

Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut)
{
    ASSERT(uvOut);

    float4 absdir = f4_abs(dir);
    float v = f4_hmax3(absdir);

    float2 uv;
    Cubeface face;
    if (v == absdir.x)
    {
        // z, y
        uv.x = dir.z;
        uv.y = dir.y;
        face = dir.x > 0.0f ? Cubeface_XP : Cubeface_XM;
    }
    else if (v == absdir.y)
    {
        // x, z
        uv.x = dir.x;
        uv.y = dir.z;
        face = dir.y > 0.0f ? Cubeface_YP : Cubeface_YM;
    }
    else
    {
        // -x, y
        uv.x = -dir.x;
        uv.y = dir.y;
        face = dir.z > 0.0f ? Cubeface_ZP : Cubeface_ZM;
    }
    uv = f2_unorm(uv);

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

    float4 pos = f4_0;

    switch (face)
    {
        default:
            ASSERT(false);
            break;

        case Cubeface_XP:
            pos.z = uv.x;
            pos.y = uv.y;
            pos.x = 1.0f;
            break;
        case Cubeface_XM:
            pos.z = -uv.x;
            pos.y = uv.y;
            pos.x = -1.0f;
            break;

        case Cubeface_YP:
            pos.x = uv.x;
            pos.z = uv.y;
            pos.y = 1.0f;
            break;
        case Cubeface_YM:
            pos.x = uv.x;
            pos.z = -uv.y;
            pos.y = -1.0f;
            break;

        case Cubeface_ZP:
            pos.x = -uv.x;
            pos.y = uv.y;
            pos.z = 1.0f;
            break;
        case Cubeface_ZM:
            pos.x = uv.x;
            pos.y = uv.y;
            pos.z = -1.0f;
            break;
    }

    return f4_normalize3(pos);
}

float4 VEC_CALL Cubemap_FaceDir(Cubeface face)
{
    float4 N = f4_0;
    switch (face)
    {
    default:
    case Cubeface_XP:
        N.x = 1.0f;
        break;
    case Cubeface_XM:
        N.x = -1.0f;
        break;
    case Cubeface_YP:
        N.y = 1.0f;
        break;
    case Cubeface_YM:
        N.y = -1.0f;
        break;
    case Cubeface_ZP:
        N.z = 1.0f;
        break;
    case Cubeface_ZM:
        N.z = -1.0f;
        break;
    }
    return N;
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

i32 Cubemap_Bake(
    Cubemap* cm,
    const struct pt_scene_s* scene,
    float4 origin,
    i32 samples,
    i32 prevSampleCount,
    i32 bounces)
{
    ASSERT(cm);
    ASSERT(scene);
    ASSERT(samples >= 0);
    ASSERT(prevSampleCount >= 0);
    ASSERT(bounces >= 0);

    ray_t ray = { .ro = origin };
    pt_raygen_t* raygen = pt_raygen(scene, ray, ptdist_sphere, samples, bounces);
    task_sys_schedule();
    task_await((task_t*)raygen);

    const int2 size = i2_s(cm->size);
    const float4* pim_noalias directions = raygen->directions;
    const float3* pim_noalias colors = raygen->colors;
    for (i32 i = 0; i < samples; ++i)
    {
        const float weight = 1.0f / (1.0f + prevSampleCount);
        float4 dir = directions[i];
        float4 rad = { colors[i].x, colors[i].y, colors[i].z };

        float2 uv;
        Cubeface face = Cubemap_CalcUv(dir, &uv);

        float4* pim_noalias buffer = cm->faces[face];
        ASSERT(buffer);

        i32 offset = UvClamp(size, uv);
        buffer[offset] = f4_lerpvs(buffer[offset], rad, weight);
    }

    return samples + prevSampleCount;
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
        const float4 L = f4_normalize3(f4_reflect3(f4_neg(N), H));
        const float NoL = f4_dot3(L, N);
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
    const float roughness = (float)mip / (float)(mipCount - 1);

    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        i32 fi = i % len;
        int2 coord = { fi % size, fi / size };

        float4 N = Cubemap_CalcDir(size, face, coord, f2_0);
        float3x3 TBN = NormalToTBN(N);
        float4 light = PrefilterEnvMap(src, TBN, N, sampleCount, roughness);
        Cubemap_WriteMip(dst, face, coord, mip, light);
    }
}

void Cubemap_Prefilter(const Cubemap* src, Cubemap* dst, u32 sampleCount)
{
    ASSERT(src);
    ASSERT(dst);

    const i32 mipCount = i1_min(5, i1_min(src->mipCount, dst->mipCount));
    const i32 size = i1_min(src->size, dst->size);

    prefilter_t* last = NULL;
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * 6;

        prefilter_t* task = tmp_calloc(sizeof(*task));
        task->src = src;
        task->dst = dst;
        task->mip = m;
        task->size = mSize;
        task->sampleCount = sampleCount;
        task_submit((task_t*)task, PrefilterFn, len);
        last = task;
    }

    if (last)
    {
        task_sys_schedule();
        task_await((task_t*)last);
    }
}
