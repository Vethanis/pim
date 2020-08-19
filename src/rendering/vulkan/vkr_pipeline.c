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

vkrPipelineLayout* vkrPipelineLayout_New(void)
{
    vkrPipelineLayout* layout = perm_calloc(sizeof(*layout));
    layout->refcount = 1;
    return layout;
}

void vkrPipelineLayout_Retain(vkrPipelineLayout* layout)
{
    if (layout && layout->refcount > 0)
    {
        layout->refcount += 1;
    }
}

void vkrPipelineLayout_Release(vkrPipelineLayout* layout)
{
    if (layout && layout->refcount > 0)
    {
        layout->refcount -= 1;
        if (layout->refcount == 0)
        {
            if (layout->handle)
            {
                vkDestroyPipelineLayout(g_vkr.dev, layout->handle, NULL);
            }
            for (i32 i = 0; i < layout->setCount; ++i)
            {
                vkDestroyDescriptorSetLayout(g_vkr.dev, layout->sets[i], NULL);
            }
            pim_free(layout->sets);
            pim_free(layout->ranges);
            memset(layout, 0, sizeof(*layout));
            pim_free(layout);
        }
    }
}

bool vkrPipelineLayout_AddSet(
    vkrPipelineLayout* layout,
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings)
{
    ASSERT(layout && layout->refcount > 0);
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
    }
    return false;
}

void vkrPipelineLayout_AddRange(
    vkrPipelineLayout* layout,
    VkPushConstantRange range)
{
    ASSERT(layout && layout->refcount > 0);
    {
        layout->rangeCount += 1;
        PermReserve(layout->ranges, layout->rangeCount);
        layout->ranges[layout->rangeCount - 1] = range;
    }
}

bool vkrPipelineLayout_Compile(vkrPipelineLayout* layout)
{
    ASSERT(layout && layout->refcount > 0);
    {
        if (layout->handle)
        {
            return true;
        }
        const VkPipelineLayoutCreateInfo createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = layout->setCount,
            .pSetLayouts = layout->sets,
            .pushConstantRangeCount = layout->rangeCount,
            .pPushConstantRanges = layout->ranges,
        };
        VkCheck(vkCreatePipelineLayout(g_vkr.dev, &createInfo, NULL, &layout->handle));
        return layout->handle != NULL;
    }
    return false;
}

vkrPipeline* vkrPipeline_NewGfx(
    const vkrFixedFuncs* fixedfuncs,
    const vkrVertexLayout* vertLayout,
    vkrPipelineLayout* layout,
    i32 shaderCount,
    const VkPipelineShaderStageCreateInfo* shaders,
    vkrRenderPass* renderPass,
    i32 subpass)
{
    if (!vkrPipelineLayout_Compile(layout))
    {
        return NULL;
    }

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
        .layout = layout->handle,
        .renderPass = renderPass->handle,
        .subpass = subpass,
    };

    VkPipeline handle = NULL;
    VkCheck(vkCreateGraphicsPipelines(g_vkr.dev, NULL, 1, &pipelineInfo, NULL, &handle));

    if (!handle)
    {
        return NULL;
    }

    vkrPipeline* pipeline = perm_calloc(sizeof(*pipeline));
    pipeline->refcount = 1;
    pipeline->handle = handle;
    pipeline->renderPass = renderPass;
    pipeline->subpass = subpass;
    pipeline->layout = layout;

    vkrRenderPass_Retain(renderPass);
    vkrPipelineLayout_Retain(layout);

    return pipeline;
}

void vkrPipeline_Retain(vkrPipeline* pipeline)
{
    if (pipeline && pipeline->refcount > 0)
    {
        pipeline->refcount += 1;
    }
}

void vkrPipeline_Release(vkrPipeline* pipeline)
{
    if (pipeline && pipeline->refcount > 0)
    {
        pipeline->refcount -= 1;
        if (pipeline->refcount == 0)
        {
            if (pipeline->handle)
            {
                vkDestroyPipeline(g_vkr.dev, pipeline->handle, NULL);
            }
            vkrRenderPass_Release(pipeline->renderPass);
            vkrPipelineLayout_Release(pipeline->layout);
            memset(pipeline, 0, sizeof(*pipeline));
            pim_free(pipeline);
        }
    }
}
