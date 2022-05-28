#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------

bool vkrImage_New(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);
void vkrImage_Release(vkrImage* image);

// turns an external VkImage handle into a vkrImage
bool vkrImage_Import(
    vkrImage* image,
    const VkImageCreateInfo* info,
    VkImage handle);

bool vkrImage_Reserve(
    vkrImage* image,
    const VkImageCreateInfo* info,
    vkrMemUsage memUsage);

void* vkrImage_Map(const vkrImage* image);
void vkrImage_Unmap(const vkrImage* image);
void vkrImage_Flush(const vkrImage* image);

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

void vkrImageView_Release(VkImageView view);
void vkrAttachment_Release(VkImageView view);

// ----------------------------------------------------------------------------

VkImageViewType vkrImage_InfoToViewType(const VkImageCreateInfo* info);
VkImageAspectFlags vkrImage_InfoToAspects(const VkImageCreateInfo* info);
bool vkrImage_InfoToViewInfo(
    const VkImageCreateInfo* info,
    VkImageViewCreateInfo* viewInfo);

PIM_C_END
