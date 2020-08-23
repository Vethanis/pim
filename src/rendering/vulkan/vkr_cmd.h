#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkCommandBuffer vkrCmdGet(vkrQueueId id, u32 tid);

// ----------------------------------------------------------------------------

VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags);
void vkrCmdPool_Del(VkCommandPool pool);
void vkrCmdPool_Reset(VkCommandPool pool, VkCommandPoolResetFlagBits flags);

VkCommandBuffer vkrCmdBuf_New(VkCommandPool pool);
void vkrCmdBuf_Del(VkCommandPool pool, VkCommandBuffer buffer);

void vkrCmdBegin(VkCommandBuffer cmdbuf, VkCommandBufferUsageFlags flags);
void vkrCmdEnd(VkCommandBuffer cmdbuf);
void vkrCmdReset(VkCommandBuffer cmdbuf, VkCommandBufferResetFlags flags);

void vkrCmdBeginRenderPass(
    VkCommandBuffer cmdbuf,
    const vkrRenderPass* pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    VkClearValue clearValue);
void vkrCmdNextSubpass(VkCommandBuffer cmdbuf);
void vkrCmdEndRenderPass(VkCommandBuffer cmdbuf);

void vkrCmdBindPipeline(VkCommandBuffer cmdbuf, const vkrPipeline* pipeline);

void vkrCmdViewport(
    VkCommandBuffer cmdbuf,
    VkViewport viewport,
    VkRect2D scissor);

void vkrCmdDraw(VkCommandBuffer cmdbuf, i32 vertexCount, i32 firstVertex);

PIM_C_END
