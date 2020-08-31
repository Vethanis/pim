#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

void vkrCreateQueues(vkr_t* vkr);
void vkrDestroyQueues(vkr_t* vkr);

// ----------------------------------------------------------------------------

bool vkrQueue_New(
    vkrQueue* queue,
    const vkrQueueSupport* support,
    vkrQueueId id);
void vkrQueue_Del(vkrQueue* queue);

VkQueueFamilyProperties* vkrEnumQueueFamilyProperties(
    VkPhysicalDevice phdev,
    u32* countOut);
vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf);

PIM_C_END
