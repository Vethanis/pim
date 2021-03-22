#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrInstance_Init(vkrSys* vkr);
void vkrInstance_Shutdown(vkrSys* vkr);

// ----------------------------------------------------------------------------

StrList vkrGetLayers(void);
StrList vkrGetInstExtensions(void);
VkLayerProperties* vkrEnumInstLayers(u32* countOut);
VkExtensionProperties* vkrEnumInstExtensions(u32* countOut);
void vkrListInstLayers(void);
void vkrListInstExtensions(void);
i32 vkrFindExtension(
    const VkExtensionProperties* props,
    u32 count,
    const char* name);
i32 vkrFindLayer(
    const VkLayerProperties* props,
    u32 count,
    const char* name);
bool vkrTryAddLayer(
    StrList* list,
    const VkLayerProperties* props,
    u32 propCount,
    const char* name);
bool vkrTryAddExtension(
    StrList* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* name);
VkInstance vkrCreateInstance(StrList extensions, StrList layers);

PIM_C_END
