#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkDebugUtilsMessengerEXT vkrCreateDebugMessenger(void);
void vkrDestroyDebugMessenger(VkDebugUtilsMessengerEXT* pMessenger);

VKAPI_ATTR VkBool32 VKAPI_CALL vkrOnVulkanMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

PIM_C_END
