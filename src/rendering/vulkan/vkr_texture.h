#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrFormatToBpp(VkFormat format);

i32 vkrTexture2D_MipCount(i32 width, i32 height);

bool vkrTexture2D_New(
    vkrTexture2D *const tex,
    i32 width,
    i32 height,
    VkFormat format,
    const void* data,
    i32 bytes);
void vkrTexture2D_Del(vkrTexture2D *const tex);
void vkrTexture2D_Release(vkrTexture2D *const tex);

VkFence vkrTexture2D_Upload(
    vkrTexture2D *const tex,
    void const *const data,
    i32 bytes);

PIM_C_END
