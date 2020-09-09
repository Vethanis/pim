#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrAllocator_New(vkrAllocator* allocator);
void vkrAllocator_Del(vkrAllocator* allocator);
void vkrAllocator_Update(vkrAllocator* allocator);
void vkrAllocator_Finalize(vkrAllocator* allocator);

void vkrReleasable_Add(vkrAllocator* allocator, const vkrReleasable* releasable);
bool vkrReleasable_Del(vkrReleasable* releasable, u32 frame);

VkFence vkrMem_Barrier(
    vkrQueueId id,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    const VkMemoryBarrier* glob,
    const VkBufferMemoryBarrier* buffer,
    const VkImageMemoryBarrier* img);

void* vkrMem_Map(VmaAllocation allocation);
void vkrMem_Unmap(VmaAllocation allocation);
void vkrMem_Flush(VmaAllocation allocation);

PIM_C_END
