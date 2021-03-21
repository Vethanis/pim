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
    float4* texels = Mem_Calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

u32* mipmap_new_c32(int2 size, EAlloc allocator)
{
    i32 len = mipmap_len(size);
    u32* texels = Mem_Calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

float* mipmap_new_f32(int2 size, EAlloc allocator)
{
    i32 len = mipmap_len(size);
    float* texels = Mem_Calloc(allocator, sizeof(texels[0]) * len);
    return texels;
}

typedef struct task_mipf4_s
{
    Task task;
    const float4* srcMip;
    float4* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipf4_t;

static void mipmap_f4fn(void* pbase, i32 begin, i32 end)
{
    task_mipf4_t* task = pbase;
    float4 const *const pim_noalias srcMip = task->srcMip;
    float4 *const pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;
    for (i32 i = begin; i < end; ++i)
    {
        int2 c = i2_mulvs(IndexToCoord(dstSize, i), 2);
        i32 ia = Clamp(srcSize, i2_v(c.x + 0, c.y + 0));
        i32 ib = Clamp(srcSize, i2_v(c.x + 1, c.y + 0));
        i32 ic = Clamp(srcSize, i2_v(c.x + 0, c.y + 1));
        i32 id = Clamp(srcSize, i2_v(c.x + 1, c.y + 1));
        float4 v0 = f4_mulvs(f4_add(srcMip[ia], srcMip[ib]), 0.5f);
        float4 v1 = f4_mulvs(f4_add(srcMip[ic], srcMip[id]), 0.5f);
        float4 v = f4_mulvs(f4_add(v0, v1), 0.5f);
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
        task_mipf4_t* task = Temp_Calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        Task_Run(&task->task, mipmap_f4fn, CalcMipLen(size, dstMip));
    }

    ProfileEnd(pm_mipmap_f4);
}

typedef struct task_mipc32_s
{
    Task task;
    const u32* srcMip;
    u32* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipc32_t;

static void mipmap_c32fn(Task* pbase, i32 begin, i32 end)
{
    task_mipc32_t* task = (task_mipc32_t*)pbase;
    u32 const *const pim_noalias srcMip = task->srcMip;
    u32 *const pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;
    for (i32 i = begin; i < end; ++i)
    {
        int2 c = i2_mulvs(IndexToCoord(dstSize, i), 2);
        i32 ia = Clamp(srcSize, i2_v(c.x + 0, c.y + 0));
        i32 ib = Clamp(srcSize, i2_v(c.x + 1, c.y + 0));
        i32 ic = Clamp(srcSize, i2_v(c.x + 0, c.y + 1));
        i32 id = Clamp(srcSize, i2_v(c.x + 1, c.y + 1));
        float4 va = ColorToLinear(srcMip[ia]);
        float4 vb = ColorToLinear(srcMip[ib]);
        float4 vc = ColorToLinear(srcMip[ic]);
        float4 vd = ColorToLinear(srcMip[id]);
        float4 v0 = f4_mulvs(f4_add(va, vb), 0.5f);
        float4 v1 = f4_mulvs(f4_add(vc, vd), 0.5f);
        float4 v = f4_mulvs(f4_add(v0, v1), 0.5f);
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
        task_mipc32_t* task = Temp_Calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        Task_Run(&task->task, mipmap_c32fn, CalcMipLen(size, dstMip));
    }
    ProfileEnd(pm_mipmap_c32);
}

typedef struct task_mipf32_s
{
    Task task;
    const float* srcMip;
    float* dstMip;
    int2 srcSize;
    int2 dstSize;
} task_mipf32_t;

static void mipmap_f32fn(Task* pbase, i32 begin, i32 end)
{
    task_mipf32_t* task = (task_mipf32_t*)pbase;
    float const *const pim_noalias srcMip = task->srcMip;
    float *const pim_noalias dstMip = task->dstMip;
    const int2 srcSize = task->srcSize;
    const int2 dstSize = task->dstSize;
    for (i32 i = begin; i < end; ++i)
    {
        int2 c = i2_mulvs(IndexToCoord(dstSize, i), 2);
        i32 ia = Clamp(srcSize, i2_v(c.x + 0, c.y + 0));
        i32 ib = Clamp(srcSize, i2_v(c.x + 1, c.y + 0));
        i32 ic = Clamp(srcSize, i2_v(c.x + 0, c.y + 1));
        i32 id = Clamp(srcSize, i2_v(c.x + 1, c.y + 1));
        float v0 = (srcMip[ia] + srcMip[ib]) * 0.5f;
        float v1 = (srcMip[ic] + srcMip[id]) * 0.5f;
        float v = (v0 + v1) * 0.5f;
        dstMip[i] = v;
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
        task_mipf32_t* task = Temp_Calloc(sizeof(*task));
        task->srcMip = mipChain + CalcMipOffset(size, srcMip);
        task->dstMip = mipChain + CalcMipOffset(size, dstMip);
        task->srcSize = CalcMipSize(size, srcMip);
        task->dstSize = CalcMipSize(size, dstMip);
        Task_Run(&task->task, mipmap_f32fn, CalcMipLen(size, dstMip));
    }
    ProfileEnd(pm_mipmap_f32);
}
