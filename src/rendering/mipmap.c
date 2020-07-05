#include "rendering/mipmap.h"
#include "math/int2_funcs.h"
#include "math/color.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/sampler.h"
#include "common/profiler.h"

i32 mipmap_len(int2 size)
{
    return MipChainLen(size);
}

float4* mipmap_new_f4(int2 size, EAlloc allocator)
{
    i32 len = mipmap_len(size);
    float4* texels = pim_calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

u32* mipmap_new_c32(int2 size, EAlloc allocator)
{
    i32 len = mipmap_len(size);
    u32* texels = pim_calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

float* mipmap_new_f32(int2 size, EAlloc allocator)
{
    i32 len = mipmap_len(size);
    float* texels = pim_calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

typedef struct task_mipf4_s
{
    task_t task;
    const float4* srcMip;
    float4* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipf4_t;

static void mipmap_f4fn(task_t* pbase, i32 begin, i32 end)
{
    task_mipf4_t* task = (task_mipf4_t*)pbase;
    const float4* pim_noalias srcMip = task->srcMip;
    float4* pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;

    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = IndexToCoord(dstSize, i);
        int2 ca = { coord.x * 2 + 0, coord.y * 2 + 0 };
        int2 cb = { coord.x * 2 + 1, coord.y * 2 + 0 };
        int2 cc = { coord.x * 2 + 0, coord.y * 2 + 1 };
        int2 cd = { coord.x * 2 + 1, coord.y * 2 + 1 };

        i32 ia = Clamp(srcSize, ca);
        i32 ib = Clamp(srcSize, cb);
        i32 ic = Clamp(srcSize, cc);
        i32 id = Clamp(srcSize, cd);

        float4 va = srcMip[ia];
        float4 vb = srcMip[ib];
        float4 vc = srcMip[ic];
        float4 vd = srcMip[id];

        float4 v = f4_add(f4_add(f4_add(f4_mulvs(va, 0.25f), f4_mulvs(vb, 0.25f)), f4_mulvs(vc, 0.25f)), f4_mulvs(vd, 0.25f));

        dstMip[i] = v;
    }
}

ProfileMark(pm_mipmap_f4, mipmap_f4)
void mipmap_f4(float4* mipChain, int2 size)
{
    ProfileBegin(pm_mipmap_f4);

    i32 mipCount = CalcMipCount(size);
    for (i32 dstMip = 1; dstMip < mipCount; ++dstMip)
    {
        i32 srcMip = dstMip - 1;
        task_mipf4_t* task = tmp_calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        task_run(&task->task, mipmap_f4fn, CalcMipLen(size, dstMip));
    }

    ProfileEnd(pm_mipmap_f4);
}

typedef struct task_mipc32_s
{
    task_t task;
    const u32* srcMip;
    u32* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipc32_t;

static void mipmap_c32fn(task_t* pbase, i32 begin, i32 end)
{
    task_mipc32_t* task = (task_mipc32_t*)pbase;
    const u32* pim_noalias srcMip = task->srcMip;
    u32* pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;

    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = IndexToCoord(dstSize, i);
        int2 ca = { coord.x * 2 + 0, coord.y * 2 + 0 };
        int2 cb = { coord.x * 2 + 1, coord.y * 2 + 0 };
        int2 cc = { coord.x * 2 + 0, coord.y * 2 + 1 };
        int2 cd = { coord.x * 2 + 1, coord.y * 2 + 1 };

        i32 ia = Clamp(srcSize, ca);
        i32 ib = Clamp(srcSize, cb);
        i32 ic = Clamp(srcSize, cc);
        i32 id = Clamp(srcSize, cd);

        float4 va = ColorToLinear(srcMip[ia]);
        float4 vb = ColorToLinear(srcMip[ib]);
        float4 vc = ColorToLinear(srcMip[ic]);
        float4 vd = ColorToLinear(srcMip[id]);

        float4 v = f4_add(f4_add(f4_add(f4_mulvs(va, 0.25f), f4_mulvs(vb, 0.25f)), f4_mulvs(vc, 0.25f)), f4_mulvs(vd, 0.25f));

        dstMip[i] = LinearToColor(v);
    }
}

ProfileMark(pm_mipmap_c32, mipmap_c32)
void mipmap_c32(u32* mipChain, int2 size)
{
    ProfileBegin(pm_mipmap_c32);
    i32 mipCount = CalcMipCount(size);
    for (i32 dstMip = 1; dstMip < mipCount; ++dstMip)
    {
        i32 srcMip = dstMip - 1;
        task_mipc32_t* task = tmp_calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        task_run(&task->task, mipmap_c32fn, CalcMipLen(size, dstMip));
    }
    ProfileEnd(pm_mipmap_c32);
}

typedef struct task_mipf32_s
{
    task_t task;
    const float* srcMip;
    float* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipf32_t;

static void mipmap_f32fn(task_t* pbase, i32 begin, i32 end)
{
    task_mipf32_t* task = (task_mipf32_t*)pbase;
    const float* pim_noalias srcMip = task->srcMip;
    float* pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;

    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = IndexToCoord(dstSize, i);
        int2 ca = { coord.x * 2 + 0, coord.y * 2 + 0 };
        int2 cb = { coord.x * 2 + 1, coord.y * 2 + 0 };
        int2 cc = { coord.x * 2 + 0, coord.y * 2 + 1 };
        int2 cd = { coord.x * 2 + 1, coord.y * 2 + 1 };

        i32 ia = Clamp(srcSize, ca);
        i32 ib = Clamp(srcSize, cb);
        i32 ic = Clamp(srcSize, cc);
        i32 id = Clamp(srcSize, cd);

        dstMip[i] = 0.25f * srcMip[ia] + 0.25f * srcMip[ib] + 0.25f * srcMip[ic] + 0.25f * srcMip[id];
    }
}

ProfileMark(pm_mipmap_f32, mipmap_f32)
void mipmap_f32(float* mipChain, int2 size)
{
    ProfileBegin(pm_mipmap_f32);
    i32 mipCount = CalcMipCount(size);
    for (i32 dstMip = 1; dstMip < mipCount; ++dstMip)
    {
        i32 srcMip = dstMip - 1;
        task_mipf32_t* task = tmp_calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        task_run(&task->task, mipmap_f32fn, CalcMipLen(size, dstMip));
    }
    ProfileEnd(pm_mipmap_f32);
}
