#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkShaderStageFlags vkrShaderTypeToStage(vkrShaderType type);

bool vkrShader_New(
    VkPipelineShaderStageCreateInfo* shader,
    const char* filename,
    const char* entrypoint,
    vkrShaderType type);

void vkrShader_Del(VkPipelineShaderStageCreateInfo* shader);

PIM_C_END
