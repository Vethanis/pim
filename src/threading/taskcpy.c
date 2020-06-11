#include "threading/taskcpy.h"
#include "threading/task.h"
#include "allocator/allocator.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"
#include <string.h>

typedef struct taskcpy_s
{
    task_t task;
    void* dst;
    const void* src;
    i32 sizeOf;
} taskcpy_t;

static void CpyFn(task_t* pbase, i32 begin, i32 end)
{
    taskcpy_t* task = (taskcpy_t*)pbase;
    u8* pim_noalias dst = task->dst;
    const u8* pim_noalias src = task->src;
    const i32 sizeOf = task->sizeOf;

    const i32 bytes = (end - begin) * sizeOf;
    const i32 offset = begin * sizeOf;

    memcpy(dst + offset, src + offset, bytes);
}

void taskcpy(void* dst, const void* src, i32 sizeOf, i32 length)
{
    if (length > 0)
    {
        ASSERT(dst);
        ASSERT(src);
        ASSERT(sizeOf > 0);

        taskcpy_t* task = tmp_calloc(sizeof(*task));
        task->dst = dst;
        task->src = src;
        task->sizeOf = sizeOf;
        task_run(&task->task, CpyFn, length);
    }
}

typedef struct blit34_s
{
    task_t task;
    float4* dst;
    const float3* src;
} blit34_t;

static void Blit34Fn(task_t* pbase, i32 begin, i32 end)
{
    blit34_t* task = (blit34_t*)pbase;
    float4* pim_noalias dst = task->dst;
    const float3* pim_noalias src = task->src;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = f3_f4(src[i], 1.0f);
    }
}

void blit_3to4(int2 size, float4* dst, const float3* src)
{
    i32 len = size.x * size.y;
    if (len > 0)
    {
        ASSERT(dst);
        ASSERT(src);

        blit34_t* task = tmp_calloc(sizeof(*task));
        task->dst = dst;
        task->src = src;
        task_run(&task->task, Blit34Fn, len);
    }
}

typedef struct blit43_s
{
    task_t task;
    float3* dst;
    const float4* src;
} blit43_t;

static void Blit43Fn(task_t* pbase, i32 begin, i32 end)
{
    blit43_t* task = (blit43_t*)pbase;
    float3* pim_noalias dst = task->dst;
    const float4* pim_noalias src = task->src;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = f4_f3(src[i]);
    }
}

void blit_4to3(int2 size, float3* dst, const float4* src)
{
    i32 len = size.x * size.y;
    if (len > 0)
    {
        ASSERT(dst);
        ASSERT(src);

        blit43_t* task = tmp_calloc(sizeof(*task));
        task->dst = dst;
        task->src = src;
        task_run(&task->task, Blit43Fn, len);
    }
}

float4* blitnew_3to4(int2 size, const float3* src, EAlloc allocator)
{
    i32 len = size.x * size.y;
    if (len > 0)
    {
        float4* dst = pim_malloc(allocator, sizeof(dst[0]) * len);
        blit_3to4(size, dst, src);
        return dst;
    }
    return NULL;
}

float3* blitnew_4to3(int2 size, const float4* src, EAlloc allocator)
{
    i32 len = size.x * size.y;
    if (len > 0)
    {
        float3* dst = pim_malloc(allocator, sizeof(dst[0]) * len);
        blit_4to3(size, dst, src);
        return dst;
    }
    return NULL;
}
