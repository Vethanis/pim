#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMainPass_New(vkrMainPass *const pass);
void vkrMainPass_Del(vkrMainPass *const pass);

void vkrMainPass_Setup(vkrMainPass *const pass);
void vkrMainPass_Execute(
    vkrMainPass *const pass,
    VkCommandBuffer cmd,
    VkFence fence);

PIM_C_END
