#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrShaderTypeToStage(vkrShaderType type);

VkPipelineShaderStageCreateInfo vkrCreateShader(vkrShaderType type, const u32* dwords, i32 length);
void vkrDestroyShader(VkPipelineShaderStageCreateInfo* info);

PIM_C_END
