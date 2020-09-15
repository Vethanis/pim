#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include "math/types.h"
#include <string.h>

i32 vkrVertTypeSize(vkrVertType type)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return 0;
    case vkrVertType_float:
        return sizeof(float);
    case vkrVertType_float2:
        return sizeof(float2);
    case vkrVertType_float3:
        return sizeof(float3);
    case vkrVertType_float4:
        return sizeof(float4);
    case vkrVertType_int:
        return sizeof(i32);
    case vkrVertType_int2:
        return sizeof(int2);
    case vkrVertType_int3:
        return sizeof(int3);
    case vkrVertType_int4:
        return sizeof(int4);
    }
}

VkFormat vkrVertTypeFormat(vkrVertType type)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return 0;
    case vkrVertType_float:
        return VK_FORMAT_R32_SFLOAT;
    case vkrVertType_float2:
        return VK_FORMAT_R32G32_SFLOAT;
    case vkrVertType_float3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case vkrVertType_float4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case vkrVertType_int:
        return VK_FORMAT_R32_SINT;
    case vkrVertType_int2:
        return VK_FORMAT_R32G32_SINT;
    case vkrVertType_int3:
        return VK_FORMAT_R32G32B32_SINT;
    case vkrVertType_int4:
        return VK_FORMAT_R32G32B32A32_SINT;
    }
}

VkDescriptorSetLayout vkrSetLayout_New(
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings,
    VkDescriptorSetLayoutCreateFlags flags)
{
    ASSERT(pBindings || !bindingCount);
    ASSERT(bindingCount >= 0);
    const VkDescriptorSetLayoutCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = flags,
        .bindingCount = bindingCount,
        .pBindings = pBindings,
    };
    VkDescriptorSetLayout handle = NULL;
    VkCheck(vkCreateDescriptorSetLayout(g_vkr.dev, &createInfo, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrSetLayout_Del(VkDescriptorSetLayout handle)
{
    if (handle)
    {
        vkDestroyDescriptorSetLayout(g_vkr.dev, handle, NULL);
    }
}

VkPipelineLayout vkrPipelineLayout_New(
    i32 setCount, const VkDescriptorSetLayout* setLayouts,
    i32 rangeCount, const VkPushConstantRange* ranges)
{
    const VkPipelineLayoutCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = setCount,
        .pSetLayouts = setLayouts,
        .pushConstantRangeCount = rangeCount,
        .pPushConstantRanges = ranges,
    };
    VkPipelineLayout handle = NULL;
    VkCheck(vkCreatePipelineLayout(g_vkr.dev, &createInfo, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrPipelineLayout_Del(VkPipelineLayout layout)
{
    if (layout)
    {
        vkDestroyPipelineLayout(g_vkr.dev, layout, NULL);
    }
}

VkPipeline vkrPipeline_NewGfx(
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    i32 subpass,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders)
{
    ASSERT(fixedfuncs);
    ASSERT(vertLayout);
    ASSERT(layout);
    ASSERT(renderPass);

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertLayout->bindingCount,
        .pVertexBindingDescriptions = vertLayout->bindings,
        .vertexAttributeDescriptionCount = vertLayout->attributeCount,
        .pVertexAttributeDescriptions = vertLayout->attributes,
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = fixedfuncs->topology,
    };
    const VkPipelineViewportStateCreateInfo viewportState =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &fixedfuncs->viewport,
        .scissorCount = 1,
        .pScissors = &fixedfuncs->scissor,
    };
    const VkPipelineRasterizationStateCreateInfo rasterizer =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = fixedfuncs->depthClamp,
        .polygonMode = fixedfuncs->polygonMode,
        .cullMode = fixedfuncs->cullMode,
        .frontFace = fixedfuncs->frontFace,
        .lineWidth = 1.0f,
    };
    const VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencil =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = fixedfuncs->depthTestEnable,
        .depthWriteEnable = fixedfuncs->depthWriteEnable,
        .depthCompareOp = fixedfuncs->depthCompareOp,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };
    const i32 attachCount = fixedfuncs->attachmentCount;
    VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = { 0 };
    for (i32 i = 0; i < attachCount; ++i)
    {
        // visual studio formatter fails at C syntax here :(
        colorBlendAttachments[i] = (VkPipelineColorBlendAttachmentState)
        {
            .blendEnable = fixedfuncs->attachments[i].blendEnable,
                .srcColorBlendFactor = fixedfuncs->attachments[i].srcColorBlendFactor,
                .dstColorBlendFactor = fixedfuncs->attachments[i].dstColorBlendFactor,
                .colorBlendOp = fixedfuncs->attachments[i].colorBlendOp,
                .srcAlphaBlendFactor = fixedfuncs->attachments[i].srcAlphaBlendFactor,
                .dstAlphaBlendFactor = fixedfuncs->attachments[i].dstAlphaBlendFactor,
                .alphaBlendOp = fixedfuncs->attachments[i].alphaBlendOp,
                .colorWriteMask = fixedfuncs->attachments[i].colorWriteMask,
        };
    }
    const VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = fixedfuncs->attachmentCount,
        .pAttachments = colorBlendAttachments,
    };
    const VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    const VkPipelineDynamicStateCreateInfo dynamicStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = NELEM(dynamicStates),
        .pDynamicStates = dynamicStates,
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shaderCount,
        .pStages = shaders,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = NULL,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicStateInfo,
        .layout = layout,
        .renderPass = renderPass,
        .subpass = subpass,
    };

    VkPipeline handle = NULL;
    VkCheck(vkCreateGraphicsPipelines(g_vkr.dev, NULL, 1, &pipelineInfo, NULL, &handle));
    ASSERT(handle);

    return handle;
}

VkPipeline vkrPipeline_NewComp(
    VkPipelineLayout layout,
    const VkPipelineShaderStageCreateInfo* shader)
{
    ASSERT(layout);
    ASSERT(shader);

    const VkComputePipelineCreateInfo pipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .flags = 0x0,
        .stage = *shader,
        .layout = layout,
    };
    VkPipeline handle = NULL;
    VkCheck(vkCreateComputePipelines(g_vkr.dev, NULL, 1, &pipelineInfo, NULL, &handle));
    ASSERT(handle);

    return handle;
}

void vkrPipeline_Del(VkPipeline pipeline)
{
    if (pipeline)
    {
        vkDestroyPipeline(g_vkr.dev, pipeline, NULL);
    }
}
