#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrAttachment_New(
    vkrAttachment* att,
    i32 width,
    i32 height,
    VkFormat format,
    VkImageUsageFlags usage);
void vkrAttachment_Del(vkrAttachment* att);

VkAttachmentDescription vkrAttachment_Desc(
    VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout,
    VkFormat format);

PIM_C_END
