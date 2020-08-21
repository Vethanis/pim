#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrInstance_Init(vkr_t* vkr);
void vkrInstance_Shutdown(vkr_t* vkr);

// ----------------------------------------------------------------------------

strlist_t vkrGetLayers(void);
strlist_t vkrGetInstExtensions(void);
VkExtensionProperties* vkrEnumInstExtensions(u32* countOut);
void vkrListInstExtensions(void);
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

PIM_C_END
