#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkSurfaceKHR vkrCreateSurface(VkInstance inst, GLFWwindow* win);

void vkrSelectFamily(i32* fam, i32 i, const VkQueueFamilyProperties* props);
vkrQueueSupport vkrQueryQueueSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf);

vkrSwapchainSupport vkrQuerySwapchainSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf);

i32 vkrSelectSwapFormat(const VkSurfaceFormatKHR* formats, u32 formatCount);
i32 vkrSelectSwapMode(const VkPresentModeKHR* modes, u32 count);
VkExtent2D vkrSelectSwapExtent(const VkSurfaceCapabilitiesKHR* caps, i32 width, i32 height);

void vkrCreateSwapchain(vkrDisplay* display, VkSwapchainKHR prev);

PIM_C_END
