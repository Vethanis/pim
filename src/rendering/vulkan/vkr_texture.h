#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexture2D_New(
    vkrTexture2D* tex,
    i32 width,
    i32 height,
    VkFormat format,
    const void* data,
    i32 bytes);
void vkrTexture2D_Del(vkrTexture2D* tex);

PIM_C_END
