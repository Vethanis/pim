#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "threading/task.h"
#include <string.h>

ProfileMark(pm_cmdget, vkrCmdGet)
VkCommandBuffer vkrCmdGet(vkrQueueId id, u32 tid)
{
    ProfileBegin(pm_cmdget);
    ASSERT(g_vkr.chain.handle);
    ASSERT(tid < (u32)g_vkr.queues[id].threadcount);
    u32 syncIndex = g_vkr.chain.syncIndex;
    VkCommandBuffer cmd = g_vkr.queues[id].buffers[syncIndex][tid];
    ProfileEnd(pm_cmdget);
    return cmd;
}

// ----------------------------------------------------------------------------

VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags)
{
    const VkCommandPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = family,
    };
    VkCommandPool pool = NULL;
    VkCheck(vkCreateCommandPool(g_vkr.dev, &poolInfo, NULL, &pool));
    ASSERT(pool);
    return pool;
}

void vkrCmdPool_Del(VkCommandPool pool)
{
    if (pool)
    {
        vkDestroyCommandPool(g_vkr.dev, pool, NULL);
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

ProfileMark(pm_cmdbufnew, vkrCmdBuf_New)
VkCommandBuffer vkrCmdBuf_New(VkCommandPool pool)
{
    ProfileBegin(pm_cmdbufnew);
    const VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer buffer = NULL;
    VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &allocInfo, &buffer));
    ASSERT(buffer);
    ProfileEnd(pm_cmdbufnew);
    return buffer;
}

ProfileMark(pm_cmdbufdel, vkrCmdBuf_Del)
void vkrCmdBuf_Del(VkCommandPool pool, VkCommandBuffer buffer)
{
    ProfileBegin(pm_cmdbufdel);
    ASSERT(pool);
    if (buffer)
    {
        vkFreeCommandBuffers(g_vkr.dev, pool, 1, &buffer);
    }
    ProfileEnd(pm_cmdbufdel);
}

ProfileMark(pm_begin, vkrCmdBegin)
void vkrCmdBegin(VkCommandBuffer buffer, VkCommandBufferUsageFlags flags)
{
    ProfileBegin(pm_begin);
    ASSERT(buffer);
    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };
    VkCheck(vkBeginCommandBuffer(buffer, &beginInfo));
    ProfileEnd(pm_begin);
}

ProfileMark(pm_end, vkrCmdEnd)
void vkrCmdEnd(VkCommandBuffer buffer)
{
    ProfileBegin(pm_end);
    ASSERT(buffer);
    VkCheck(vkEndCommandBuffer(buffer));
    ProfileEnd(pm_end);
}

ProfileMark(pm_reset, vkrCmdReset)
void vkrCmdReset(VkCommandBuffer buffer, VkCommandBufferResetFlags flags)
{
    ProfileBegin(pm_reset);
    ASSERT(buffer);
    VkCheck(vkResetCommandBuffer(buffer, flags));
    ProfileEnd(pm_reset);
}

ProfileMark(pm_beginrenderpass, vkrCmdBeginRenderPass)
void vkrCmdBeginRenderPass(
    VkCommandBuffer cmdbuf,
    const vkrRenderPass* pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    VkClearValue clearValue)
{
    ProfileBegin(pm_beginrenderpass);
    ASSERT(cmdbuf);
    ASSERT(vkrAlive(pass));
    ASSERT(pass->handle);
    ASSERT(framebuf);
    const VkRenderPassBeginInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = pass->handle,
        .framebuffer = framebuf,
        .renderArea = rect,
        .clearValueCount = 1,
        .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(cmdbuf, &info, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_beginrenderpass);
}

ProfileMark(pm_nextsubpass, vkrCmdNextSubpass)
void vkrCmdNextSubpass(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_nextsubpass);
    ASSERT(cmdbuf);
    vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_nextsubpass);
}

ProfileMark(pm_endrenderpass, vkrCmdEndRenderPass)
void vkrCmdEndRenderPass(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_endrenderpass);
    ASSERT(cmdbuf);
    vkCmdEndRenderPass(cmdbuf);
    ProfileEnd(pm_endrenderpass);
}

ProfileMark(pm_bindpipeline, vkrCmdBindPipeline)
void vkrCmdBindPipeline(VkCommandBuffer cmdbuf, const vkrPipeline* pipeline)
{
    ProfileBegin(pm_bindpipeline);
    ASSERT(cmdbuf);
    ASSERT(vkrAlive(pipeline));
    vkCmdBindPipeline(cmdbuf, pipeline->bindpoint, pipeline->handle);
    ProfileEnd(pm_bindpipeline);
}

ProfileMark(pm_viewport, vkrCmdViewport)
void vkrCmdViewport(
    VkCommandBuffer cmdbuf,
    VkViewport viewport,
    VkRect2D scissor)
{
    ProfileBegin(pm_viewport);
    ASSERT(cmdbuf);
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
    ProfileEnd(pm_viewport);
}

ProfileMark(pm_draw, vkrCmdDraw)
void vkrCmdDraw(VkCommandBuffer cmdbuf, i32 vertexCount, i32 firstVertex)
{
    ProfileBegin(pm_draw);
    ASSERT(cmdbuf);
    vkCmdDraw(cmdbuf, vertexCount, 1, firstVertex, 0);
    ProfileEnd(pm_draw);
}
