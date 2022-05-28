#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrFormatToBpp(VkFormat format);

i32 vkrTexture_MipCount(i32 width, i32 height, i32 depth);

bool vkrTexture_New(
    vkrImage *const image,
    VkImageViewType viewType,
    VkFormat format,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips);
void vkrTexture_Release(vkrImage *const image);

void vkrTexture_Upload(
    vkrImage *const image,
    i32 layer,
    void const *const data,
    i32 bytes);

PIM_C_END
