#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMainPass_New(vkrPipeline* pipeline);
void vkrMainPass_Del(vkrPipeline* pipeline);

void vkrMainPass_Draw(
    vkrPipeline* pipeline,
    VkCommandBuffer cmd,
    VkFence fence);

PIM_C_END
