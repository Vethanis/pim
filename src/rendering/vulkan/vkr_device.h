#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkExtensionProperties* vkrEnumInstExtensions(u32* countOut);
VkExtensionProperties* vkrEnumDevExtensions(
    VkPhysicalDevice phdev,
    u32* countOut);

u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** pDevices, // optional
    VkPhysicalDeviceFeatures** pFeatures, // optional
    VkPhysicalDeviceProperties** pProps); // optional

void vkrListInstExtensions(void);
void vkrListDevExtensions(VkPhysicalDevice phdev);
strlist_t vkrGetLayers(void);
strlist_t vkrGetInstExtensions(void);
strlist_t vkrGetDevExtensions(VkPhysicalDevice phdev);
i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* extName);
bool vkrTryAddExtension(
    strlist_t* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* extName);

VkInstance vkrCreateInstance(strlist_t extensions, strlist_t layers);

VkPhysicalDevice vkrSelectPhysicalDevice(
    VkPhysicalDeviceProperties* propsOut, // optional
    VkPhysicalDeviceFeatures* featuresOut); // optional

VkDevice vkrCreateDevice(
    strlist_t extensions,
    strlist_t layers,
    VkQueue* queuesOut, // optional
    VkQueueFamilyProperties* propsOut); // optional

PIM_C_END
