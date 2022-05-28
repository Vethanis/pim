#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrBindings_Init(void);
void vkrBindings_Update(void);
void vkrBindings_Shutdown(void);

VkDescriptorSetLayout vkrBindings_GetSetLayout(void);
VkDescriptorSet vkrBindings_GetSet(void);

void vkrBindings_BindImage(
    i32 id,
    VkDescriptorType type,
    VkSampler sampler,
    VkImageView view,
    VkImageLayout layout);

void vkrBindings_BindBuffer(
    i32 id,
    VkDescriptorType type,
    vkrBuffer const *const buffer);

PIM_C_END
