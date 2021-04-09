#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------

bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);
void vkrImage_Release(vkrImage* image);

bool vkrImage_Reserve(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);

void* vkrImage_Map(const vkrImage* image);
void vkrImage_Unmap(const vkrImage* image);
void vkrImage_Flush(const vkrImage* image);

void vkrImage_Barrier(
    vkrImage* image,
    VkCommandBuffer cmd,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages);

void vkrImage_Transfer(
    vkrImage* image,
    vkrQueueId srcQueueId, vkrQueueId dstQueueId,
    VkCommandBuffer srcCmd, VkCommandBuffer dstCmd,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages);

// ----------------------------------------------------------------------------

bool vkrImageSet_New(
    vkrImageSet* set,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);
void vkrImageSet_Release(vkrImageSet* set);

vkrImage* vkrImageSet_Current(vkrImageSet* set);
vkrImage* vkrImageSet_Prev(vkrImageSet* set);

bool vkrImageSet_Reserve(
    vkrImageSet* set,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);

// ----------------------------------------------------------------------------

VkImageView vkrImageView_New(
    VkImage image,
    VkImageViewType type,
    VkFormat format,
    VkImageAspectFlags aspect,
    i32 baseMip, i32 mipCount,
    i32 baseLayer, i32 layerCount);
void vkrImageView_Release(VkImageView view);

// ----------------------------------------------------------------------------

PIM_C_END
