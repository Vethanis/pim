#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMemSys_Init(void);
void vkrMemSys_Shutdown(void);
void vkrMemSys_Update(void);
void vkrMemSys_Finalize(void);

bool vkrMem_BufferNew(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage);
void vkrMem_BufferDel(vkrBuffer *const buffer);

bool vkrMem_ImageNew(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);
void vkrMem_ImageDel(vkrImage* image);

void vkrReleasable_Add(vkrReleasable const *const releasable);
bool vkrReleasable_Del(vkrReleasable *const releasable, u32 frame);

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
