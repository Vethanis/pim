#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexTable_Init(void);
void vkrTexTable_Update(void);
void vkrTexTable_Shutdown(void);

void vkrTexTable_Write(VkDescriptorSet set, i32 binding);

bool vkrTexTable_Exists(vkrTextureId id);
vkrTextureId vkrTexTable_Alloc(
    i32 width,
    i32 height,
    VkFormat format,
    void const *const data);
bool vkrTexTable_Free(vkrTextureId id);
VkFence vkrTexTable_Upload(
    vkrTextureId id,
    void const *const data,
    i32 bytes);

PIM_C_END
