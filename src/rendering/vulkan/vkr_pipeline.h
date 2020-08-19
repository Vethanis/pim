#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef enum
{
    vkrVertType_float,
    vkrVertType_float2,
    vkrVertType_float3,
    vkrVertType_float4,
    vkrVertType_int,
    vkrVertType_int2,
    vkrVertType_int3,
    vkrVertType_int4,
} vkrVertType;

// vkr only supports 'SoA' layout (1 binding per attribute, no interleaving)
// with a maximum of 8 streams, in sequential location
typedef struct vkrVertexLayout
{
    i32 streamCount;
    vkrVertType types[8];
} vkrVertexLayout;

typedef struct vkrBlendState
{
    VkColorComponentFlags colorWriteMask;
    bool blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
} vkrBlendState;

typedef struct vkrFixedFuncs
{
    VkViewport viewport;
    VkRect2D scissor;
    VkPrimitiveTopology topology;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    VkCompareOp depthCompareOp;
    bool scissorOn;
    bool depthClamp;
    bool depthTestEnable;
    bool depthWriteEnable;
    i32 attachmentCount;
    vkrBlendState attachments[8];
} vkrFixedFuncs;

typedef struct vkrPipelineLayout
{
    VkPipelineLayout layout;
    VkDescriptorSetLayout* sets;
    VkPushConstantRange* ranges;
    i32 setCount;
    i32 rangeCount;
} vkrPipelineLayout;

typedef struct vkrPipeline
{
    VkPipeline pipeline;
    vkrPipelineLayout layout;
    VkRenderPass renderPass;
    i32 subpass;
} vkrPipeline;

// ----------------------------------------------------------------------------

i32 vkrVertTypeSize(vkrVertType type);
VkFormat vkrVertTypeFormat(vkrVertType type);

void vkrPipelineLayout_New(vkrPipelineLayout* layout);
void vkrPipelineLayout_Del(vkrPipelineLayout* layout);
bool vkrPipelineLayout_AddSet(
    vkrPipelineLayout* layout,
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings);
void vkrPipelineLayout_AddRange(
    vkrPipelineLayout* layout,
    VkPushConstantRange range);

bool vkrPipeline_NewGfx(
    vkrPipeline* pipeline,
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    vkrPipelineLayout* layout, // moved into pipeline's ownership
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders,
    VkRenderPass renderPass, // moved into pipeline's ownership
    i32 subpass);
void vkrPipeline_Del(vkrPipeline* pipeline);

PIM_C_END

