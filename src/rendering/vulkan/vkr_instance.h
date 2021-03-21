#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrInstance_Init(vkrSys* vkr);
void vkrInstance_Shutdown(vkrSys* vkr);

// ----------------------------------------------------------------------------

strlist_t vkrGetLayers(void);
strlist_t vkrGetInstExtensions(void);
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
    strlist_t* list,
    const VkLayerProperties* props,
    u32 propCount,
    const char* name);
bool vkrTryAddExtension(
    strlist_t* list,
    const VkExtensionProperties* props,
    u32 propCount,
    const char* name);
VkInstance vkrCreateInstance(strlist_t extensions, strlist_t layers);

PIM_C_END
