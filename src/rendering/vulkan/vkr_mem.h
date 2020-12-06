#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrAllocator_New(vkrAllocator *const allocator);
void vkrAllocator_Del(vkrAllocator *const allocator);
void vkrAllocator_Update(vkrAllocator *const allocator);
void vkrAllocator_Finalize(vkrAllocator *const allocator);

void vkrReleasable_Add(
    vkrAllocator *const allocator,
    vkrReleasable const *const releasable);
bool vkrReleasable_Del(
    vkrReleasable *const releasable,
    u32 frame);

VkFence vkrMem_Barrier(
    vkrQueueId id,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    const VkMemoryBarrier* glob,
    const VkBufferMemoryBarrier* buffer,
    const VkImageMemoryBarrier* img);

void *const vkrMem_Map(VmaAllocation allocation);
void vkrMem_Unmap(VmaAllocation allocation);
void vkrMem_Flush(VmaAllocation allocation);

PIM_C_END
