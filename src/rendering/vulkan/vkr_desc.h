#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkDescriptorPool vkrDescPool_New(
    i32 maxSets,
    i32 sizeCount,
    const VkDescriptorPoolSize* sizes);
void vkrDescPool_Del(VkDescriptorPool pool);

PIM_C_END
