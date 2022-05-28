#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrScreenBlit_New(void);
void vkrScreenBlit_Del(void);

void vkrScreenBlit_Blit(
    const void* src,
    i32 width,
    i32 height,
    VkFormat format);

PIM_C_END
