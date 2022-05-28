#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrFramebuffer_Init(void);
void vkrFramebuffer_Shutdown(void);

// find or create a framebuffer matching the inputs
VkFramebuffer vkrFramebuffer_Get(
    const vkrImage** attachments,
    i32 count,
    i32 width,
    i32 height);

// remove all framebuffers referencing the given view
void vkrFramebuffer_Remove(VkImageView view);

PIM_C_END
