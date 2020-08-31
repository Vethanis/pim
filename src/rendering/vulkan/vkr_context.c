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
    ASSERT(g_vkr.chain.handle);
    ASSERT(tid < g_vkr.context.threadcount);
    return &g_vkr.context.threads[tid].frames[syncIndex];
}

void vkrContext_New(vkrContext* ctx)
{
    ASSERT(ctx);
    memset(ctx, 0, sizeof(*ctx));
    i32 threadcount = task_thread_ct();
    ctx->threadcount = threadcount;
    ctx->threads = perm_calloc(sizeof(ctx->threads[0]) * threadcount);
    for (i32 tr = 0; tr < threadcount; ++tr)
    {
        vkrThreadContext_New(&ctx->threads[tr]);
    }
}

void vkrContext_Del(vkrContext* ctx)
{
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
}

void vkrThreadContext_New(vkrThreadContext* ctx)
{
    ASSERT(ctx);
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        vkrFrameContext_New(&ctx->frames[i]);
    }
}

void vkrThreadContext_Del(vkrThreadContext* ctx)
{
    ASSERT(ctx);
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        vkrFrameContext_Del(&ctx->frames[i]);
    }
}

void vkrFrameContext_New(vkrFrameContext* ctx)
{
    ASSERT(ctx);
    memset(ctx, 0, sizeof(*ctx));
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        i32 family = g_vkr.queues[id].family;
        const u32 flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ctx->cmdpools[id] = vkrCmdPool_New(family, flags);
        ctx->cmdbufs[id] = vkrCmdBuf_New(ctx->cmdpools[id], id);
    }
    const VkDescriptorPoolSize poolSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1, // one big ssbo?
        },
    };
    ctx->descpool = vkrDescPool_New(1, NELEM(poolSizes), poolSizes);
    vkrBuffer_New(
        &ctx->perdrawbuf,
        sizeof(vkrPerDraw) * kDrawsPerThread,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        vkrMemUsage_CpuToGpu);
    vkrBuffer_New(
        &ctx->percambuf,
        sizeof(vkrPerCamera) * kCamerasPerThread,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        vkrMemUsage_CpuToGpu);
}

void vkrFrameContext_Del(vkrFrameContext* ctx)
{
    if (ctx)
    {
        vkrBuffer_Del(&ctx->percambuf);
        vkrBuffer_Del(&ctx->perdrawbuf);
        vkrDescPool_Del(ctx->descpool);
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            vkrCmdBuf_Del(&ctx->cmdbufs[id]);
            vkrCmdPool_Del(ctx->cmdpools[id]);
        }

        memset(ctx, 0, sizeof(*ctx));
    }
}

void vkrContext_WritePerDraw(
    vkrFrameContext* ctx,
    const vkrPerDraw* perDraws,
    i32 length)
{
    ASSERT(ctx);
    ASSERT(length <= kDrawsPerThread);
    ASSERT(length >= 0);
    if ((length > 0) && (length <= kDrawsPerThread))
    {
        vkrBuffer_Write(
            &ctx->perdrawbuf,
            perDraws,
            sizeof(perDraws[0]) * length);
    }
}

void vkrContext_WritePerCamera(
    vkrFrameContext* ctx,
    const vkrPerCamera* perCameras,
    i32 length)
{
    ASSERT(ctx);
    ASSERT(length <= kCamerasPerThread);
    ASSERT(length >= 0);
    if ((length > 0) && (length <= kCamerasPerThread))
    {
        vkrBuffer_Write(
            &ctx->percambuf,
            perCameras,
            sizeof(perCameras[0]) * length);
    }
}
