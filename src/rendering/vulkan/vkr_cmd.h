#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags);
void vkrCmdPool_Del(VkCommandPool pool);
void vkrCmdPool_Reset(VkCommandPool pool, VkCommandPoolResetFlagBits flags);

// ----------------------------------------------------------------------------

bool vkrCmdAlloc_New(
    vkrCmdAlloc* allocator,
    const vkrQueue* queue,
    VkCommandBufferLevel level);
void vkrCmdAlloc_Del(vkrCmdAlloc* allocator);

void vkrCmdAlloc_Reserve(vkrCmdAlloc* allocator, u32 capacity);

// used for temporary primary command buffers (not the presentation command buffer)
VkCommandBuffer vkrCmdAlloc_GetTemp(vkrCmdAlloc* allocator, VkFence* fenceOut);

// for secondary command buffers executed within a primary buffer
VkCommandBuffer vkrCmdAlloc_GetSecondary(vkrCmdAlloc* allocator, VkFence primaryFence);

// ----------------------------------------------------------------------------

void vkrCmdBegin(VkCommandBuffer cmdbuf);
void vkrCmdBeginSec(
    VkCommandBuffer cmd,
    VkRenderPass renderPass,
    i32 subpass,
    VkFramebuffer framebuffer);
void vkrCmdEnd(VkCommandBuffer cmdbuf);
void vkrCmdReset(VkCommandBuffer cmdbuf);

void vkrCmdSubmit(
    VkQueue queue,
    VkCommandBuffer cmd,
    VkFence fence,                  // optional
    VkSemaphore waitSema,           // optional
    VkPipelineStageFlags waitMask,  // optional
    VkSemaphore signalSema);        // optional

void vkrCmdBeginRenderPass(
    VkCommandBuffer cmdbuf,
    VkRenderPass pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    i32 clearCount,
    const VkClearValue* clearValues,
    VkSubpassContents contents);
void vkrCmdExecCmds(VkCommandBuffer cmd, i32 count, const VkCommandBuffer* pSecondaries);
void vkrCmdNextSubpass(VkCommandBuffer cmdbuf, VkSubpassContents contents);
void vkrCmdEndRenderPass(VkCommandBuffer cmdbuf);

void vkrCmdBindPipeline(VkCommandBuffer cmdbuf, const vkrPipeline* pipeline);

void vkrCmdViewport(
    VkCommandBuffer cmdbuf,
    VkViewport viewport,
    VkRect2D scissor);

void vkrCmdDraw(VkCommandBuffer cmdbuf, i32 vertexCount, i32 firstVertex);
void vkrCmdDrawMesh(VkCommandBuffer cmdbuf, const vkrMesh* mesh);

void vkrCmdCopyBuffer(VkCommandBuffer cmdbuf, vkrBuffer src, vkrBuffer dst);

void vkrCmdBufferBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier* barrier);
void vkrCmdImageBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier* barrier);

void vkrCmdPushConstants(
    VkCommandBuffer cmdbuf,
    VkPipelineLayout layout,
    VkShaderStageFlags stages,
    const void* dwords,
    i32 bytes);

void vkrCmdBindDescSets(
    VkCommandBuffer cmdbuf,
    const vkrPipeline* pipeline,
    i32 setCount,
    const VkDescriptorSet* sets);

PIM_C_END
