#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDevice_Init(vkrSys* vkr);
void vkrDevice_Shutdown(vkrSys* vkr);
void vkrDevice_WaitIdle(void);

// ----------------------------------------------------------------------------

VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut);
void vkrListDevExtensions(VkPhysicalDevice phdev);
StrList vkrGetDevExtensions(VkPhysicalDevice phdev);

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
    StrList extensions,
    StrList layers);

PIM_C_END
