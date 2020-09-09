#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrVertTypeSize(vkrVertType type);
VkFormat vkrVertTypeFormat(vkrVertType type);

void vkrPipelineLayout_New(vkrPipelineLayout* layout);
void vkrPipelineLayout_Del(vkrPipelineLayout* layout);
bool vkrPipelineLayout_AddSet(
    vkrPipelineLayout* layout,
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings,
    VkDescriptorSetLayoutCreateFlags flags);
void vkrPipelineLayout_AddRange(
    vkrPipelineLayout* layout,
    VkPushConstantRange range);
bool vkrPipelineLayout_Compile(vkrPipelineLayout* layout);

bool vkrPipeline_NewGfx(
    vkrPipeline* pipeline,
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    vkrPipelineLayout* layout,
    VkRenderPass renderPass,
    i32 subpass,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders);

void vkrPipeline_Del(vkrPipeline* pipeline);

PIM_C_END

