#include "rendering/vulkan/vkr_pipeline.h"
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

void vkrPipelineLayout_New(vkrPipelineLayout* layout)
{
    memset(layout, 0, sizeof(*layout));
}

void vkrPipelineLayout_Del(vkrPipelineLayout* layout)
{
    if (layout)
    {
        if (layout->layout)
        {
            vkDestroyPipelineLayout(g_vkr.dev, layout->layout, NULL);
        }
        for (i32 i = 0; i < layout->setCount; ++i)
        {
            vkDestroyDescriptorSetLayout(g_vkr.dev, layout->sets[i], NULL);
        }
        pim_free(layout->sets);
        pim_free(layout->ranges);
        memset(layout, 0, sizeof(*layout));
    }
}

bool vkrPipelineLayout_AddSet(
    vkrPipelineLayout* layout,
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings)
{
    VkDescriptorSetLayout set = NULL;
    const VkDescriptorSetLayoutCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindingCount,
        .pBindings = pBindings,
    };
    VkCheck(vkCreateDescriptorSetLayout(g_vkr.dev, &createInfo, NULL, &set));
    if (set != NULL)
    {
        layout->setCount += 1;
        PermReserve(layout->sets, layout->setCount);
        layout->sets[layout->setCount - 1] = set;
        return true;
    }
    return false;
}

void vkrPipelineLayout_AddRange(
    vkrPipelineLayout* layout,
    VkPushConstantRange range)
{
    layout->rangeCount += 1;
    PermReserve(layout->ranges, layout->rangeCount);
    layout->ranges[layout->rangeCount - 1] = range;
}

bool vkrPipeline_NewGfx(
    vkrPipeline* pipeline,
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    vkrPipelineLayout* layout,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders,
    VkRenderPass renderPass,
    i32 subpass)
{
    memset(pipeline, 0, sizeof(*pipeline));

    VkVertexInputBindingDescription vertBindings[8] = { 0 };
    VkVertexInputAttributeDescription vertAttribs[8] = { 0 };
    for (i32 i = 0; i < vertLayout->streamCount; ++i)
    {
        vkrVertType vtype = vertLayout->types[i];
        i32 size = vkrVertTypeSize(vtype);
        VkFormat format = vkrVertTypeFormat(vtype);
        vertBindings[i].binding = i;
        vertBindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertBindings[i].stride = size;
        vertAttribs[i].binding = i;
        vertAttribs[i].format = format;
        vertAttribs[i].location = i;
        vertAttribs[i].offset = 0;
    }
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertLayout->streamCount,
        .pVertexBindingDescriptions = vertBindings,
        .vertexAttributeDescriptionCount = vertLayout->streamCount,
        .pVertexAttributeDescriptions = vertAttribs,
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
        .scissorCount = fixedfuncs->scissorOn ? 1 : 0,
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
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = { 0 };
    for (i32 i = 0; i < fixedfuncs->attachmentCount; ++i)
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

    const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layout->setCount,
        .pSetLayouts = layout->sets,
        .pushConstantRangeCount = layout->rangeCount,
        .pPushConstantRanges = layout->ranges,
    };
    VkCheck(vkCreatePipelineLayout(g_vkr.dev, &pipelineLayoutInfo, NULL, &layout->layout));
    if (layout->layout == NULL)
    {
        vkrPipelineLayout_Del(layout);
        return false;
    }

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
        .layout = layout->layout,
        .renderPass = renderPass,
        .subpass = subpass,
    };

    VkPipeline handle = NULL;
    VkCheck(vkCreateGraphicsPipelines(g_vkr.dev, NULL, 1, &pipelineInfo, NULL, &handle));

    if (!handle)
    {
        vkrPipelineLayout_Del(layout);
        return false;
    }

    pipeline->pipeline = handle;
    pipeline->renderPass = renderPass;
    pipeline->subpass = subpass;
    pipeline->layout = *layout;
    memset(layout, 0, sizeof(*layout));

    return true;
}

void vkrPipeline_Del(vkrPipeline* pipeline)
{
    if (pipeline)
    {
        if (pipeline->pipeline)
        {
            vkDestroyPipeline(g_vkr.dev, pipeline->pipeline, NULL);
        }
        if (pipeline->renderPass)
        {
            vkDestroyRenderPass(g_vkr.dev, pipeline->renderPass, NULL);
        }
        vkrPipelineLayout_Del(&pipeline->layout);
        memset(pipeline, 0, sizeof(*pipeline));
    }
}
