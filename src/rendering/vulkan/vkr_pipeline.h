#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrVertTypeSize(vkrVertType type);
VkFormat vkrVertTypeFormat(vkrVertType type);

VkPipelineLayout vkrPipelineLayout_New(
    i32 setCount, const VkDescriptorSetLayout* setLayouts,
    i32 rangeCount, const VkPushConstantRange* ranges);
void vkrPipelineLayout_Del(VkPipelineLayout layout);

VkPipeline vkrPipeline_NewGfx(
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    i32 subpass,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders);

VkPipeline vkrPipeline_NewComp(
    VkPipelineLayout layout,
    const VkPipelineShaderStageCreateInfo* shader);

void vkrPipeline_Del(VkPipeline pipeline);

PIM_C_END

