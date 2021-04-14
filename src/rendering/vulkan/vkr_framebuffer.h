#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrFramebuffer_New(
    vkrFramebuffer* fbuf,
    const VkImageView* attachments,
    const VkFormat* formats,
    i32 attachmentCount,
    i32 width,
    i32 height);
void vkrFramebuffer_Del(vkrFramebuffer* fbuf);

PIM_C_END
