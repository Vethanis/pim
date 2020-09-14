#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMainPass_New(vkrMainPass* pass);
void vkrMainPass_Del(vkrMainPass* pass);

void vkrMainPass_Draw(
    vkrMainPass* pass,
    VkCommandBuffer cmd,
    VkFence fence);

PIM_C_END
