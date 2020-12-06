#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrBuffer_New(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage);
void vkrBuffer_Del(vkrBuffer *const buffer);

// destroys the resource after kFramesInFlight
// if fence is provided, it is used instead
void vkrBuffer_Release(vkrBuffer *const buffer, VkFence fence);

bool vkrBufferSet_New(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage);
void vkrBufferSet_Del(vkrBufferSet *const set);
void vkrBufferSet_Release(vkrBufferSet *const set, VkFence fence);
vkrBuffer *const vkrBufferSet_Current(vkrBufferSet *const set);
vkrBuffer *const vkrBufferSet_Prev(vkrBufferSet *const set);

// if size exceeds buffer's current size:
// - creates new larger buffer
// - releases old buffer with fence
// - updates buffer inout argument
// - contents of buffer are invalidated by this call!
bool vkrBuffer_Reserve(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    VkFence fence);

void *const vkrBuffer_Map(vkrBuffer const *const buffer);
void vkrBuffer_Unmap(vkrBuffer const *const buffer);
void vkrBuffer_Flush(vkrBuffer const *const buffer);

// helper for map, unmap, flush
void vkrBuffer_Write(
    vkrBuffer const *const buffer,
    void const *const src,
    i32 size);

void vkrBuffer_Barrier(
    vkrBuffer *const buffer,
    VkCommandBuffer cmd,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask);

void vkrBuffer_Transfer(
    vkrBuffer *const buffer,
    vkrQueueId srcQueueId,
    vkrQueueId dstQueueId,
    VkCommandBuffer srcCmd,
    VkCommandBuffer dstCmd,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask);

PIM_C_END
