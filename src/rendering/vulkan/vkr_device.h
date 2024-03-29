#pragma once

#include "rendering/vulkan/vkr.h"
#include "containers/strlist.h"

PIM_C_BEGIN

bool vkrDevice_Init(vkrSys* vkr);
void vkrDevice_Shutdown(vkrSys* vkr);
void vkrDevice_WaitIdle(void);

// ----------------------------------------------------------------------------

u32 vkrEnumPhysicalDevices(
    VkInstance inst,
    VkPhysicalDevice** devicesOut,
    vkrProps** propsOut,
    vkrFeats** featuresOut,
    vkrDevExts** extsOut);
VkPhysicalDevice vkrSelectPhysicalDevice(
    const vkrWindow* window,
    vkrProps* propsOut,
    vkrFeats* featsOut,
    vkrDevExts* extsOut);

VkDevice vkrCreateDevice(
    const vkrWindow* window,
    StrList extensions,
    StrList layers);

PIM_C_END
