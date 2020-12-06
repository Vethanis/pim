#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrSampler_Init(void);
void vkrSampler_Update(void);
void vkrSampler_Shutdown(void);

VkSampler vkrSampler_Get(
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso);

PIM_C_END
