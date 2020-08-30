#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "math/types.h"
#include "threading/task.h"
#include <string.h>

ProfileMark(pm_cmdpoolnew, vkrCmdPool_New)
VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags)
{
    ProfileBegin(pm_cmdpoolnew);
    const VkCommandPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = family,
    };
    VkCommandPool pool = NULL;
    VkCheck(vkCreateCommandPool(g_vkr.dev, &poolInfo, NULL, &pool));
    ASSERT(pool);
    ProfileEnd(pm_cmdpoolnew);
    return pool;
}

ProfileMark(pm_cmdpooldel, vkrCmdPool_Del)
void vkrCmdPool_Del(VkCommandPool pool)
{
    if (pool)
    {
        ProfileBegin(pm_cmdpooldel);
        vkDestroyCommandPool(g_vkr.dev, pool, NULL);
        ProfileEnd(pm_cmdpooldel);
    }
}

ProfileMark(pm_cmdpoolreset, vkrCmdPool_Reset)
void vkrCmdPool_Reset(VkCommandPool pool, VkCommandPoolResetFlagBits flags)
{
    ProfileBegin(pm_cmdpoolreset);
    ASSERT(pool);
    VkCheck(vkResetCommandPool(g_vkr.dev, pool, flags));
    ProfileEnd(pm_cmdpoolreset);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_cmdget, vkrCmdGet)
vkrCmdBuf* vkrCmdGet(vkrQueueId id)
{
    ProfileBegin(pm_cmdget);
    ASSERT(g_vkr.chain.handle);
    i32 tid = task_thread_id();
    ASSERT(tid < g_vkr.queues[id].threadcount);
    u32 syncIndex = g_vkr.chain.syncIndex;
    ASSERT(syncIndex < kFramesInFlight);
    vkrCmdBuf* cmdbuf = &(g_vkr.queues[id].buffers[syncIndex][tid]);
    ASSERT(cmdbuf->handle);
    ProfileEnd(pm_cmdget);
    return cmdbuf;
}

ProfileMark(pm_cmdsubmit, vkrCmdSubmit)
void vkrCmdSubmit(
    vkrQueueId id,
    VkCommandBuffer cmd,
    VkFence fence,
    VkSemaphore waitSema,
    VkPipelineStageFlags waitMask,
    VkSemaphore signalSema)
{
    ProfileBegin(pm_cmdsubmit);
    ASSERT(cmd);
    VkQueue submitQueue = g_vkr.queues[id].handle;
    ASSERT(submitQueue);
    const VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = waitSema ? 1 : 0,
        .pWaitSemaphores = &waitSema,
        .pWaitDstStageMask = &waitMask,
        .signalSemaphoreCount = signalSema ? 1 : 0,
        .pSignalSemaphores = &signalSema,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkCheck(vkQueueSubmit(submitQueue, 1, &submitInfo, fence));
    ProfileEnd(pm_cmdsubmit);
}

void vkrCmdSubmit2(vkrCmdBuf* cmdbuf)
{
    vkrCmdAwait(cmdbuf);
    if (cmdbuf->state == vkrCmdState_Executable)
    {
        vkrCmdSubmit(
            cmdbuf->queue,
            cmdbuf->handle,
            cmdbuf->fence,
            NULL,
            0x0,
            NULL);
        cmdbuf->state = vkrCmdState_Pending;
    }
    else
    {
        ASSERT(false);
    }
}

ProfileMark(pm_cmdbufnew, vkrCmdBuf_New)
vkrCmdBuf vkrCmdBuf_New(VkCommandPool pool, vkrQueueId id)
{
    ProfileBegin(pm_cmdbufnew);
    const VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer handle = NULL;
    VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &allocInfo, &handle));
    ASSERT(handle);
    vkrCmdBuf cmdbuf =
    {
        .handle = handle,
        .pool = pool,
        .fence = vkrFence_New(false),
        .queue = id,
        .state = vkrCmdState_Initial,
    };
    ProfileEnd(pm_cmdbufnew);
    return cmdbuf;
}

ProfileMark(pm_cmdbufdel, vkrCmdBuf_Del)
void vkrCmdBuf_Del(vkrCmdBuf* cmdbuf)
{
    if (cmdbuf)
    {
        ProfileBegin(pm_cmdbufdel);
        if (cmdbuf->handle)
        {
            ASSERT(cmdbuf->pool);
            vkrCmdAwait(cmdbuf);
            vkFreeCommandBuffers(g_vkr.dev, cmdbuf->pool, 1, &cmdbuf->handle);
        }
        vkrFence_Del(cmdbuf->fence);
        memset(cmdbuf, 0, sizeof(*cmdbuf));
        ProfileEnd(pm_cmdbufdel);
    }
}

ProfileMark(pm_begin, vkrCmdBegin)
void vkrCmdBegin(vkrCmdBuf* cmdbuf)
{
    vkrCmdAwait(cmdbuf);
    if (cmdbuf->state != vkrCmdState_Recording)
    {
        ProfileBegin(pm_begin);
        const VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0x0,
        };
        VkCheck(vkBeginCommandBuffer(cmdbuf->handle, &beginInfo));
        cmdbuf->state = vkrCmdState_Recording;
        ProfileEnd(pm_begin);
    }
}

ProfileMark(pm_end, vkrCmdEnd)
void vkrCmdEnd(vkrCmdBuf* cmdbuf)
{
    ASSERT(cmdbuf);
    ASSERT(cmdbuf->handle);
    if (cmdbuf->state == vkrCmdState_Recording)
    {
        ProfileBegin(pm_end);
        VkCheck(vkEndCommandBuffer(cmdbuf->handle));
        cmdbuf->state = vkrCmdState_Executable;
        ProfileEnd(pm_end);
    }
    else
    {
        ASSERT(false);
    }
}

ProfileMark(pm_reset, vkrCmdReset)
void vkrCmdReset(vkrCmdBuf* cmdbuf)
{
    vkrCmdAwait(cmdbuf);
    if (cmdbuf->state != vkrCmdState_Initial)
    {
        ProfileBegin(pm_reset);
        VkCheck(vkResetCommandBuffer(cmdbuf->handle, 0x0));
        cmdbuf->state = vkrCmdState_Initial;
        ProfileEnd(pm_reset);
    }
}

ProfileMark(pm_await, vkrCmdAwait)
void vkrCmdAwait(vkrCmdBuf* cmdbuf)
{
    ASSERT(cmdbuf);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->fence);
    if (cmdbuf->state == vkrCmdState_Pending)
    {
        ProfileBegin(pm_await);
        vkrFence_Wait(cmdbuf->fence);
        vkrFence_Reset(cmdbuf->fence);
        cmdbuf->state = vkrCmdState_Executable;
        ProfileEnd(pm_await);
    }
}

ProfileMark(pm_beginrenderpass, vkrCmdBeginRenderPass)
void vkrCmdBeginRenderPass(
    vkrCmdBuf* cmdbuf,
    const vkrRenderPass* pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    VkClearValue clearValue)
{
    ProfileBegin(pm_beginrenderpass);
    ASSERT(cmdbuf->handle);
    ASSERT(vkrAlive(pass));
    ASSERT(pass->handle);
    ASSERT(framebuf);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    const VkRenderPassBeginInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = pass->handle,
        .framebuffer = framebuf,
        .renderArea = rect,
        .clearValueCount = 1,
        .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(cmdbuf->handle, &info, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_beginrenderpass);
}

ProfileMark(pm_nextsubpass, vkrCmdNextSubpass)
void vkrCmdNextSubpass(vkrCmdBuf* cmdbuf)
{
    ProfileBegin(pm_nextsubpass);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    vkCmdNextSubpass(cmdbuf->handle, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_nextsubpass);
}

ProfileMark(pm_endrenderpass, vkrCmdEndRenderPass)
void vkrCmdEndRenderPass(vkrCmdBuf* cmdbuf)
{
    ProfileBegin(pm_endrenderpass);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    vkCmdEndRenderPass(cmdbuf->handle);
    ProfileEnd(pm_endrenderpass);
}

ProfileMark(pm_bindpipeline, vkrCmdBindPipeline)
void vkrCmdBindPipeline(vkrCmdBuf* cmdbuf, const vkrPipeline* pipeline)
{
    ProfileBegin(pm_bindpipeline);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    ASSERT(vkrAlive(pipeline));
    vkCmdBindPipeline(cmdbuf->handle, pipeline->bindpoint, pipeline->handle);
    ProfileEnd(pm_bindpipeline);
}

ProfileMark(pm_viewport, vkrCmdViewport)
void vkrCmdViewport(
    vkrCmdBuf* cmdbuf,
    VkViewport viewport,
    VkRect2D scissor)
{
    ProfileBegin(pm_viewport);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    vkCmdSetViewport(cmdbuf->handle, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf->handle, 0, 1, &scissor);
    ProfileEnd(pm_viewport);
}

ProfileMark(pm_draw, vkrCmdDraw)
void vkrCmdDraw(vkrCmdBuf* cmdbuf, i32 vertexCount, i32 firstVertex)
{
    ProfileBegin(pm_draw);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    vkCmdDraw(cmdbuf->handle, vertexCount, 1, firstVertex, 0);
    ProfileEnd(pm_draw);
}

ProfileMark(pm_drawmesh, vkrCmdDrawMesh)
void vkrCmdDrawMesh(vkrCmdBuf* cmdbuf, const vkrMesh* mesh)
{
    ProfileBegin(pm_drawmesh);

    ASSERT(mesh);
    ASSERT(cmdbuf);
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    ASSERT(mesh->vertCount >= 0);
    ASSERT(mesh->indexCount >= 0);
    ASSERT(mesh->buffer.handle);

    if (mesh->vertCount > 0)
    {
        const VkDeviceSize streamSize = sizeof(float4) * mesh->vertCount;
        const VkDeviceSize indexOffset = streamSize * vkrMeshStream_COUNT;
        const VkBuffer buffers[] =
        {
            mesh->buffer.handle,
            mesh->buffer.handle,
            mesh->buffer.handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
            streamSize * 1,
            streamSize * 2,
        };
        vkCmdBindVertexBuffers(cmdbuf->handle, 0, NELEM(buffers), buffers, offsets);
        if (mesh->indexCount > 0)
        {
            vkCmdBindIndexBuffer(cmdbuf->handle, mesh->buffer.handle, indexOffset, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(cmdbuf->handle, mesh->indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(cmdbuf->handle, mesh->vertCount, 1, 0, 0);
        }
    }

    ProfileEnd(pm_drawmesh);
}

ProfileMark(pm_copybuffer, vkrCmdCopyBuffer)
void vkrCmdCopyBuffer(vkrCmdBuf* cmdbuf, vkrBuffer src, vkrBuffer dst)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    ASSERT(src.handle);
    ASSERT(dst.handle);
    i32 size = src.size < dst.size ? src.size : dst.size;
    ASSERT(size >= 0);
    if (size > 0)
    {
        ProfileBegin(pm_copybuffer);
        const VkBufferCopy region =
        {
            .size = size,
        };
        vkCmdCopyBuffer(cmdbuf->handle, src.handle, dst.handle, 1, &region);
        ProfileEnd(pm_copybuffer);
    }
}

void vkrCmdBufferBarrier(
    vkrCmdBuf* cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier* barrier)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf->handle,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        1, barrier,
        0, NULL);
}

void vkrCmdImageBarrier(vkrCmdBuf* cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier* barrier)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->state == vkrCmdState_Recording);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf->handle,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        0, NULL,
        1, barrier);
}
