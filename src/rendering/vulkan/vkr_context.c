#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_mem.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

vkrFrameContext* vkrContext_Get(void)
{
    i32 tid = task_thread_id();
    u32 syncIndex = g_vkr.chain.syncIndex;
    ASSERT(tid < g_vkr.context.threadcount);
    vkrFrameContext* ctx = &g_vkr.context.threads[tid].frames[syncIndex];
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
    if (!vkrBuffer_New(&ctx->percambuf, sizeof(vkrPerCamera), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vkrMemUsage_CpuToGpu, PIM_FILELINE))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrBuffer_New(&ctx->perdrawbuf, sizeof(vkrPerDraw) * 256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vkrMemUsage_CpuToGpu, PIM_FILELINE))
    {
        success = false;
        goto cleanup;
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
        vkrBuffer_Del(&ctx->percambuf);
        vkrBuffer_Del(&ctx->perdrawbuf);
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
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        if (!vkrFrameContext_New(&ctx->frames[i]))
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
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrFrameContext_Del(&ctx->frames[i]);
        }
    }
    ProfileEnd(pm_trctxdel);
}

ProfileMark(pm_frctxnew, vkrFrameContext_New)
bool vkrFrameContext_New(vkrFrameContext* ctx)
{
    ProfileBegin(pm_frctxnew);
    bool success = true;
    ASSERT(ctx);
    memset(ctx, 0, sizeof(*ctx));
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
    const VkDescriptorPoolSize poolSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 8,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 8,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 8,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 8,
        },
    };
    VkDescriptorPool descpool = vkrDescPool_New(1, NELEM(poolSizes), poolSizes);
    ctx->descpool = descpool;
    ASSERT(descpool);
    if (!descpool)
    {
        success = false;
        goto cleanup;
    }
cleanup:
    if (!success)
    {
        vkrFrameContext_Del(ctx);
    }
    ProfileEnd(pm_frctxnew);
    return success;
}

ProfileMark(pm_frctxdel, vkrFrameContext_Del)
void vkrFrameContext_Del(vkrFrameContext* ctx)
{
    ProfileBegin(pm_frctxdel);
    if (ctx)
    {
        vkrDescPool_Del(ctx->descpool);
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            vkrCmdAlloc_Del(&ctx->cmds[id]);
            vkrCmdAlloc_Del(&ctx->seccmds[id]);
        }
        memset(ctx, 0, sizeof(*ctx));
    }
    ProfileEnd(pm_frctxdel);
}

void vkrContext_GetCmd(
    vkrFrameContext* ctx,
    vkrQueueId id,
    VkCommandBuffer* cmdOut,
    VkFence* fenceOut,
    VkQueue* queueOut)
{
    ASSERT(ctx);
    vkrCmdAlloc_Get(&ctx->cmds[id], cmdOut, fenceOut);
    if (queueOut)
    {
        *queueOut = ctx->cmds[id].queue;
        ASSERT(*queueOut);
    }
}

void vkrContext_GetSecCmd(
    vkrFrameContext* ctx,
    vkrQueueId id,
    VkCommandBuffer* cmdOut,
    VkFence* fenceOut,
    VkQueue* queueOut)
{
    ASSERT(ctx);
    vkrCmdAlloc_Get(&ctx->seccmds[id], cmdOut, fenceOut);
    if (queueOut)
    {
        *queueOut = ctx->seccmds[id].queue;
        ASSERT(*queueOut);
    }
}
