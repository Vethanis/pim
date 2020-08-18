#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef enum
{
    vkrShaderType_Vert,
    vkrShaderType_Frag,
    vkrShaderType_Comp,
    vkrShaderType_Raygen,
    vkrShaderType_AnyHit,
    vkrShaderType_ClosestHit,
    vkrShaderType_Miss,
    vkrShaderType_Isect,
    vkrShaderType_Call,
} vkrShaderType;

VkShaderModule vkrCreateShaderModule(const u32* dwords, i32 length);
void vkrDestroyShaderModule(VkShaderModule mod);

VkPipelineShaderStageCreateInfo vkrCreateShader(vkrShaderType type, const u32* dwords, i32 length);
void vkrDestroyShader(VkPipelineShaderStageCreateInfo* info);

PIM_C_END
