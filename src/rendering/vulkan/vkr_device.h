#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDevice_Init(vkr_t* vkr);
void vkrDevice_Shutdown(vkr_t* vkr);
void vkrDevice_WaitIdle(void);

// ----------------------------------------------------------------------------

VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut);
void vkrListDevExtensions(VkPhysicalDevice phdev);
strlist_t vkrGetDevExtensions(VkPhysicalDevice phdev);

u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** pDevices, // optional
    VkPhysicalDeviceFeatures** pFeatures, // optional
    VkPhysicalDeviceProperties** pProps); // optional
VkPhysicalDevice vkrSelectPhysicalDevice(
    const vkrDisplay* display,
    VkPhysicalDeviceProperties* propsOut, // optional
    VkPhysicalDeviceFeatures* featuresOut); // optional

VkDevice vkrCreateDevice(
    const vkrDisplay* display,
    strlist_t extensions,
    strlist_t layers);

PIM_C_END
