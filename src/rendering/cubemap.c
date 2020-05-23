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

float4 VEC_CALL Cubemap_Read(const Cubemap* cm, float4 dir, float mip)
{
    ASSERT(cm);

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

typedef struct bake_s
{
    task_t task;
    float4 origin;
    Cubemap* cm;
    const pt_scene_t* scene;
    float sampleWeight;
} bake_t;

static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    bake_t* task = (bake_t*)pBase;
    const pt_scene_t* scene = task->scene;
    Cubemap* cm = task->cm;

    const float sampleWeight = task->sampleWeight;
    const i32 size = cm->size;
    const i32 size2 = size * size;

    prng_t rng = prng_get();

    ray_t ray;
    ray.ro = task->origin;

    for (i32 i = begin; i < end; ++i)
    {
        const i32 face = i / size2;
        const i32 fi = i % size2;

        const i32 y = fi / size;
        const i32 x = fi % size;

        const int2 coord = { x, y };
        float2 Xi = f2_tent(f2_rand(&rng));
        ray.rd = Cubemap_CalcDir(size, face, coord, Xi);

        float4 rad = pt_trace_frag(&rng, scene, ray, 10);

        float4* pim_noalias buffer = cm->faces[face];
        ASSERT(buffer);
        buffer[fi] = f4_lerpvs(buffer[fi], rad, sampleWeight);
    }

    prng_set(rng);
}

struct task_s* Cubemap_Bake(
    Cubemap* cm,
    const struct pt_scene_s* scene,
    float4 origin,
    float sampleWeight)
{
    ASSERT(cm);
    ASSERT(scene);

    bake_t* task = tmp_calloc(sizeof(*task));
    task->cm = cm;
    task->scene = scene;
    task->origin = origin;
    task->sampleWeight = sampleWeight;
    i32 size = cm->size;
    task_submit((task_t*)task, BakeFn, size * size * 6);

    return (task_t*)task;
}
