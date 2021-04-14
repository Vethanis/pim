#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMainPass_New(void);
void vkrMainPass_Del(void);

void vkrMainPass_Setup(void);
void vkrMainPass_Execute(
    VkCommandBuffer cmd,
    VkFence fence);

PIM_C_END
