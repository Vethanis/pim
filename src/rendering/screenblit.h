#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrScreenBlit_New(vkrScreenBlit* blit);
void vkrScreenBlit_Del(vkrScreenBlit* blit);

void vkrScreenBlit_Blit(
    vkrScreenBlit* blit,
    VkCommandBuffer cmd,
    VkImage dstImage,
    u32 const *const pim_noalias texels,
    i32 width,
    i32 height);

PIM_C_END
