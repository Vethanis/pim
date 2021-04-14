#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrSwapchain_New(vkrSwapchain* chain, vkrDisplay* display, vkrSwapchain* prev);
void vkrSwapchain_Del(vkrSwapchain* chain);

bool vkrSwapchain_Recreate(
    vkrSwapchain* chain,
    vkrDisplay* display);

// acquire synchronization index for a frame in flight
// should be done at beginning of frame
u32 vkrSwapchain_AcquireSync(vkrSwapchain* chain, VkCommandBuffer* cmdOut, VkFence* fenceOut);
// acquire image index for a swapchain image
// should be done for presentation render pass
u32 vkrSwapchain_AcquireImage(vkrSwapchain* chain, VkFramebuffer* bufferOut);
// submits final render pass to the swapchain
void vkrSwapchain_Submit(vkrSwapchain* chain, VkCommandBuffer cmd);
// presents final render pass to the swapchain
void vkrSwapchain_Present(vkrSwapchain* chain);

VkViewport vkrSwapchain_GetViewport(const vkrSwapchain* chain);
VkRect2D vkrSwapchain_GetRect(const vkrSwapchain* chain);
float vkrSwapchain_GetAspect(const vkrSwapchain* chain);

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
