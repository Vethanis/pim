#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

i32 vkrVertTypeSize(vkrVertType type);
VkFormat vkrVertTypeFormat(vkrVertType type);

vkrPipelineLayout* vkrPipelineLayout_New(void);
void vkrPipelineLayout_Retain(vkrPipelineLayout* layout);
void vkrPipelineLayout_Release(vkrPipelineLayout* layout);
bool vkrPipelineLayout_AddSet(
    vkrPipelineLayout* layout,
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings);
void vkrPipelineLayout_AddRange(
    vkrPipelineLayout* layout,
    VkPushConstantRange range);
bool vkrPipelineLayout_Compile(vkrPipelineLayout* layout);

vkrPipeline* vkrPipeline_NewGfx(
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    vkrPipelineLayout* layout,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders,
    vkrRenderPass* renderPass,
    i32 subpass);

void vkrPipeline_Retain(vkrPipeline* pipeline);
void vkrPipeline_Release(vkrPipeline* pipeline);

PIM_C_END

