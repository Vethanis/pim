#include "rendering/clear_tile.h"
#include "math/float4_funcs.h"
#include "rendering/framebuffer.h"
#include "common/profiler.h"
#include "rendering/sampler.h"
#include "rendering/constants.h"
#include "threading/task.h"
#include "allocator/allocator.h"

typedef struct clear_s
{
    task_t task;
    framebuf_t* target;
    float4 color;
    float depth;
} clear_t;

pim_optimize
static void ClearFn(task_t* pbase, i32 begin, i32 end)
{
    clear_t* task = (clear_t*)pbase;
    float4 clearColor = task->color;
    float clearDepth = task->depth;

    framebuf_t* pim_noalias target = task->target;
    float4* pim_noalias colorBuf = target->light;
    float* pim_noalias depthBuf = target->depth;

    for (i32 i = begin; i < end; ++i)
    {
        colorBuf[i] = clearColor;
    }
    for (i32 i = begin; i < end; ++i)
    {
        depthBuf[i] = clearDepth;
    }
}

ProfileMark(pm_ClearTile, ClearTile)
pim_optimize
task_t* ClearTile(framebuf_t* target, float4 color, float depth)
{
    ProfileBegin(pm_ClearTile);
    ASSERT(target);

    clear_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->color = color;
    task->depth = depth;
    task_submit(&task->task, ClearFn, target->width * target->height);

    ProfileEnd(pm_ClearTile);

    return &task->task;
}
