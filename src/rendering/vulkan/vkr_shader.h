#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrShaderTypeToStage(vkrShaderType type);

VkPipelineShaderStageCreateInfo vkrCreateShader(const vkrCompileOutput* output);
void vkrDestroyShader(VkPipelineShaderStageCreateInfo* info);

PIM_C_END
