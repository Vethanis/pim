#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags);
void vkrCmdPool_Del(VkCommandPool pool);
void vkrCmdPool_Reset(VkCommandPool pool, VkCommandPoolResetFlagBits flags);

// ----------------------------------------------------------------------------

vkrCmdBuf vkrCmdBuf_New(VkCommandPool pool, vkrQueueId id);
void vkrCmdBuf_Del(vkrCmdBuf* cmdbuf);

vkrCmdBuf* vkrCmdGet(vkrQueueId id);
void vkrCmdBegin(vkrCmdBuf* cmdbuf);
void vkrCmdEnd(vkrCmdBuf* cmdbuf);
void vkrCmdReset(vkrCmdBuf* cmdbuf);
void vkrCmdAwait(vkrCmdBuf* cmdbuf);

void vkrCmdSubmit(
    vkrQueueId id,
    VkCommandBuffer cmd,
    VkFence fence,                  // optional
    VkSemaphore waitSema,           // optional
    VkPipelineStageFlags waitMask,  // optional
    VkSemaphore signalSema);        // optional
void vkrCmdSubmit2(vkrCmdBuf* cmdbuf);

void vkrCmdBeginRenderPass(
    vkrCmdBuf* cmdbuf,
    const vkrRenderPass* pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    VkClearValue clearValue);
void vkrCmdNextSubpass(vkrCmdBuf* cmdbuf);
void vkrCmdEndRenderPass(vkrCmdBuf* cmdbuf);

void vkrCmdBindPipeline(vkrCmdBuf* cmdbuf, const vkrPipeline* pipeline);

void vkrCmdViewport(
    vkrCmdBuf* cmdbuf,
    VkViewport viewport,
    VkRect2D scissor);

void vkrCmdDraw(vkrCmdBuf* cmdbuf, i32 vertexCount, i32 firstVertex);
void vkrCmdDrawMesh(vkrCmdBuf* cmdbuf, const vkrMesh* mesh);

void vkrCmdCopyBuffer(vkrCmdBuf* cmdbuf, vkrBuffer src, vkrBuffer dst);

void vkrCmdBufferBarrier(
    vkrCmdBuf* cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier* barrier);
void vkrCmdImageBarrier(
    vkrCmdBuf* cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier* barrier);

PIM_C_END
