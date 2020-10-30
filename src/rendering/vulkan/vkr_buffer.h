#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrBuffer_New(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    const char* tag);
void vkrBuffer_Del(vkrBuffer* buffer);

// destroys the resource after kFramesInFlight
// if fence is provided, it is used instead
void vkrBuffer_Release(vkrBuffer* buffer, VkFence fence);

// if size exceeds buffer's current size:
// - creates new larger buffer
// - releases old buffer with fence
// - updates buffer inout argument
// - contents of buffer are invalidated by this call!
bool vkrBuffer_Reserve(
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    VkFence fence,
    const char* tag);

void* vkrBuffer_Map(const vkrBuffer* buffer);
void vkrBuffer_Unmap(const vkrBuffer* buffer);
void vkrBuffer_Flush(const vkrBuffer* buffer);

// helper for map, unmap, flush
void vkrBuffer_Write(const vkrBuffer* buffer, const void* src, i32 size);

void vkrBuffer_Barrier(
    vkrBuffer* buffer,
    VkCommandBuffer cmd,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask);

void vkrBuffer_Transfer(
    vkrBuffer* buffer,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    vkrQueueId srcQueueId,
    vkrQueueId dstQueueId);

PIM_C_END
