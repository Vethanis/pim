#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

// single descriptors
typedef enum vkrBindId
{
    // uniforms
    vkrBindId_CameraData,

    // storage images
    vkrBindId_LumTexture,

    // storage buffers
    vkrBindId_HistogramBuffer,
    vkrBindId_ExposureBuffer,

    vkrBindId_COUNT
} vkrBindId;

// descriptor arrays
enum vkrBindTableId
{
    vkrBindTableId_Texture1D = vkrBindId_COUNT,
    vkrBindTableId_Texture2D,
    vkrBindTableId_Texture3D,
    vkrBindTableId_TextureCube,
    vkrBindTableId_Texture2DArray,
} vkrBindTableId;

bool vkrBindings_Init(void);
void vkrBindings_Update(void);
void vkrBindings_Shutdown(void);

VkDescriptorSetLayout vkrBindings_GetSetLayout(void);
VkDescriptorSet vkrBindings_GetSet(void);

void vkrBindings_BindImage(
    vkrBindId id,
    VkDescriptorType type,
    VkSampler sampler,
    VkImageView view,
    VkImageLayout layout);

void vkrBindings_BindBuffer(
    vkrBindId id,
    VkDescriptorType type,
    vkrBuffer const *const buffer);

PIM_C_END
