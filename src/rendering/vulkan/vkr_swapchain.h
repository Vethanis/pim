#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrSwapchain_New(vkrSwapchain* chain, vkrDisplay* display, vkrSwapchain* prev);
void vkrSwapchain_Del(vkrSwapchain* chain);
void vkrSwapchain_SetupBuffers(vkrSwapchain* chain, VkRenderPass presentPass);

bool vkrSwapchain_Recreate(
    vkrSwapchain* chain,
    vkrDisplay* display,
    VkRenderPass presentPass);

void vkrSwapchain_Acquire(
    vkrSwapchain* chain,
    u32* pSyncIndex,
    u32* pImageIndex);
void vkrSwapchain_Present(
    vkrSwapchain* chain,
    vkrCmdBuf* cmd);

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
