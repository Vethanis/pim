#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

void vkrCreateQueues(vkr_t* vkr);
void vkrDestroyQueues(vkr_t* vkr);

// ----------------------------------------------------------------------------

VkQueueFamilyProperties* vkrEnumQueueFamilyProperties(
    VkPhysicalDevice phdev,
    u32* countOut);
vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf);

PIM_C_END
