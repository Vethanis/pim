#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

vkrThreadContext* vkrContext_Get(void)
{
    i32 tid = task_thread_id();
    ASSERT(tid < g_vkr.context.threadcount);
    vkrThreadContext* ctx = &g_vkr.context.threads[tid];
    return ctx;
}

ProfileMark(pm_ctxnew, vkrContext_New)
bool vkrContext_New(vkrContext* ctx)
{
    ProfileBegin(pm_ctxnew);
    bool success = true;
    ASSERT(ctx);
    memset(ctx, 0, sizeof(*ctx));
    i32 threadcount = task_thread_ct();
    ctx->threadcount = threadcount;
    ctx->threads = perm_calloc(sizeof(ctx->threads[0]) * threadcount);
    for (i32 tr = 0; tr < threadcount; ++tr)
    {
        if (!vkrThreadContext_New(&ctx->threads[tr]))
        {
            success = false;
            goto cleanup;
        }
    }
cleanup:
    if (!success)
    {
        vkrContext_Del(ctx);
    }
    ProfileEnd(pm_ctxnew);
    return success;
}

ProfileMark(pm_ctxdel, vkrContext_New)
void vkrContext_Del(vkrContext* ctx)
{
    ProfileBegin(pm_ctxdel);
    if (ctx)
    {
        i32 threadcount = ctx->threadcount;
        for (i32 tr = 0; tr < threadcount; ++tr)
        {
            vkrThreadContext_Del(&ctx->threads[tr]);
        }
        pim_free(ctx->threads);
        memset(ctx, 0, sizeof(*ctx));
    }
    ProfileEnd(pm_ctxdel);
}

ProfileMark(pm_trctxnew, vkrThreadContext_New)
bool vkrThreadContext_New(vkrThreadContext* ctx)
{
    ProfileBegin(pm_trctxnew);
    bool success = true;
    ASSERT(ctx);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        if (!vkrCmdAlloc_New(
            &ctx->cmds[id],
            &g_vkr.queues[id],
            VK_COMMAND_BUFFER_LEVEL_PRIMARY))
        {
            success = false;
            goto cleanup;
        }
        if (!vkrCmdAlloc_New(
            &ctx->seccmds[id],
            &g_vkr.queues[id],
            VK_COMMAND_BUFFER_LEVEL_SECONDARY))
        {
            success = false;
            goto cleanup;
        }
    }
cleanup:
    if (!success)
    {
        vkrThreadContext_Del(ctx);
    }
    ProfileEnd(pm_trctxnew);
    return success;
}

ProfileMark(pm_trctxdel, vkrThreadContext_Del)
void vkrThreadContext_Del(vkrThreadContext* ctx)
{
    ProfileBegin(pm_trctxdel);
    if (ctx)
    {
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            vkrCmdAlloc_Del(&ctx->cmds[id]);
            vkrCmdAlloc_Del(&ctx->seccmds[id]);
        }
    }
    ProfileEnd(pm_trctxdel);
}

void vkrContext_OnSwapRecreate(vkrContext* ctx)
{
    const i32 threadcount = ctx->threadcount;
    vkrThreadContext* threads = ctx->threads;
    for (i32 i = 0; i < threadcount; ++i)
    {
        vkrThreadContext_OnSwapRecreate(&threads[i]);
    }
}

void vkrThreadContext_OnSwapRecreate(vkrThreadContext* ctx)
{
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkrCmdAlloc_OnSwapRecreate(&ctx->cmds[id]);
        vkrCmdAlloc_OnSwapRecreate(&ctx->seccmds[id]);
    }
}

VkCommandBuffer vkrContext_GetTmpCmd(
    vkrQueueId id,
    VkFence *const fenceOut,
    VkQueue *const queueOut)
{
    vkrThreadContext *const ctx = vkrContext_Get();
    ASSERT(queueOut);
    VkQueue queue = ctx->cmds[id].queue;
    ASSERT(queue);
    *queueOut = queue;
    return vkrCmdAlloc_GetTemp(&ctx->cmds[id], fenceOut);
}

VkCommandBuffer vkrContext_GetSecCmd(
    vkrQueueId id,
    VkFence primaryFence)
{
    vkrThreadContext *const ctx = vkrContext_Get();
    ASSERT(primaryFence);
    return vkrCmdAlloc_GetSecondary(&ctx->seccmds[id], primaryFence);
}
