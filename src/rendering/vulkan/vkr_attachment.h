#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrAttachment_New(
    vkrImage* att,
    i32 width,
    i32 height,
    VkFormat format,
    VkImageUsageFlags usage);
void vkrAttachment_Release(vkrImage* att);

PIM_C_END
