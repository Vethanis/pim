#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrSwapchain_New(vkrSwapchain* chain, vkrSwapchain* prev);
void vkrSwapchain_Del(vkrSwapchain* chain);

bool vkrSwapchain_Recreate(void);

// acquire synchronization index for a frame in flight
// should be done at beginning of frame
u32 vkrSwapchain_AcquireSync(void);
// acquire image index for a swapchain image
// should be done for presentation render pass
u32 vkrSwapchain_AcquireImage(void);
// submits then presents final render pass to the swapchain
void vkrSwapchain_Submit(vkrCmdBuf* cmd);

VkViewport vkrSwapchain_GetViewport(void);
VkRect2D vkrSwapchain_GetRect(void);
float vkrSwapchain_GetAspect(void);

// ----------------------------------------------------------------------------

vkrSwapchainSupport vkrQuerySwapchainSupport(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf);
VkSurfaceFormatKHR vkrSelectSwapFormat(
    const VkSurfaceFormatKHR* formats,
    i32 count);
VkPresentModeKHR vkrSelectSwapMode(
    const VkPresentModeKHR* modes,
    i32 count);
VkExtent2D vkrSelectSwapExtent(
    const VkSurfaceCapabilitiesKHR* caps,
    i32 width,
    i32 height);

PIM_C_END
