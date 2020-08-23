#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrSwapchain* vkrSwapchain_New(vkrDisplay* display, vkrSwapchain* prev);
void vkrSwapchain_Retain(vkrSwapchain* chain);
void vkrSwapchain_Release(vkrSwapchain* chain);
void vkrSwapchain_SetupBuffers(vkrSwapchain* chain, vkrRenderPass* presentPass);

vkrSwapchain* vkrSwapchain_Recreate(
    vkrDisplay* display,
    vkrSwapchain* prev,
    vkrRenderPass* presentPass);

u32 vkrSwapchain_Acquire(vkrSwapchain* chain, vkrSwapFrame* dstFrame);
void vkrSwapchain_Present(
    vkrSwapchain* chain,
    vkrQueueId cmdQueue,
    VkCommandBuffer cmd);

VkViewport vkrSwapchain_GetViewport(const vkrSwapchain* chain);
VkRect2D vkrSwapchain_GetRect(const vkrSwapchain* chain);

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
