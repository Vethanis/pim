#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexTable_Init(void);
void vkrTexTable_Update(void);
void vkrTexTable_Shutdown(void);

void vkrTexTable_Write(VkDescriptorSet set);

bool vkrTexTable_Exists(vkrTextureId id);
vkrTextureId vkrTexTable_Alloc(
    VkImageViewType type,
    VkFormat format,
    VkSamplerAddressMode clamp,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips);
bool vkrTexTable_Free(vkrTextureId id);
bool vkrTexTable_Upload(
    vkrTextureId id,
    i32 layer,
    void const *const data,
    i32 bytes);

bool vkrTexTable_SetSampler(
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso);

PIM_C_END
