#include "rendering/vulkan/vkr_imgui.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "ui/cimgui.h"
#include <string.h>

// ----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static const u32 __glsl_shader_vert_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
    0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
    0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
    0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
    0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
    0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
    0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
    0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
    0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
    0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
    0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
    0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
    0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
    0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
    0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
    0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
    0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
    0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
    0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
    0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
    0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
    0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
    0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
    0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
    0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
    0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
    0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
    0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
    0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
    0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
    0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
    0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
    0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
    0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
    0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static const u32 __glsl_shader_frag_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
    0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
    0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
    0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
    0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
    0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
    0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
    0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
    0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
    0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
    0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
    0x00010038
};

// ----------------------------------------------------------------------------

static void vkrImGui_SetupRenderState(
    vkrImGui* imgui,
    const ImDrawData* draw_data,
    VkCommandBuffer command_buffer,
    i32 fb_width,
    i32 fb_height);
static void vkrImGui_UploadRenderDrawData(vkrImGui* imgui, ImDrawData* draw_data);
static void vkrImGui_RenderDrawData(
    vkrImGui* imgui,
    ImDrawData* draw_data,
    VkCommandBuffer command_buffer);

// ----------------------------------------------------------------------------

bool vkrImGuiPass_New(vkrImGui* imgui, VkRenderPass renderPass)
{
    ASSERT(imgui);
    ASSERT(g_vkr.dev);
    ASSERT(renderPass);
    memset(imgui, 0, sizeof(*imgui));

    VkShaderModule vert_module = NULL;
    VkShaderModule frag_module = NULL;

    // Setup back-end capabilities flags
    {
        ImGuiIO* io = igGetIO();
        io->BackendRendererName = "vkrImGui";
        io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    }

    // Create Mesh Buffers:
    for (i32 i = 0; i < NELEM(imgui->vertbufs); ++i)
    {
        vkrBuffer_New(
            &imgui->vertbufs[i],
            1024,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            vkrMemUsage_CpuToGpu,
            PIM_FILELINE);
        vkrBuffer_New(
            &imgui->indbufs[i],
            1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            vkrMemUsage_CpuToGpu,
            PIM_FILELINE);
    }

    // Create The Shader Modules:
    {
        const VkShaderModuleCreateInfo vert_info =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(__glsl_shader_vert_spv),
            .pCode = __glsl_shader_vert_spv,
        };
        VkCheck(vkCreateShaderModule(g_vkr.dev, &vert_info, NULL, &vert_module));
        const VkShaderModuleCreateInfo frag_info =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(__glsl_shader_frag_spv),
            .pCode = __glsl_shader_frag_spv,
        };
        VkCheck(vkCreateShaderModule(g_vkr.dev, &frag_info, NULL, &frag_module));
    }

    // Create Font Texture:
    {
        ImGuiIO* io = igGetIO();

        u8* pixels = NULL;
        i32 width = 0;
        i32 height = 0;
        i32 bpp = 0;
        ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, &bpp);
        const i32 upload_size = width * height * bpp;

        ASSERT(pixels);
        ASSERT(upload_size > 0);
        vkrTexture2D_New(
            &imgui->font,
            width,
            height,
            VK_FORMAT_R8G8B8A8_SRGB,
            pixels,
            upload_size);

        // Store our identifier
        io->Fonts->TexID = (ImTextureID)imgui->font.image.handle;
    }

    // Create Descriptor Pool:
    {
        const VkDescriptorPoolSize sizes[] =
        {
            {
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
            },
        };
        imgui->descPool = vkrDescPool_New(1, NELEM(sizes), sizes);
    }

    // Create Descriptor Set Layout:
    {
        const VkSampler samplers[] = { imgui->font.sampler };
        const VkDescriptorSetLayoutBinding bindings[] =
        {
            {
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = NELEM(samplers),
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = samplers,
            },
        };
        const VkDescriptorSetLayoutCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = NELEM(bindings),
            .pBindings = bindings,
        };
        imgui->setLayout = vkrSetLayout_New(NELEM(bindings), bindings, 0x0);
    }

    // Create Descriptor Set:
    {
        const VkDescriptorSetAllocateInfo alloc_info =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = imgui->descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &imgui->setLayout,
        };
        imgui->set = vkrDesc_New(imgui->descPool, imgui->setLayout);
    }

    // Update the Descriptor Set:
    {
        const VkDescriptorImageInfo desc_images[] =
        {
            {
                .sampler = imgui->font.sampler,
                .imageView = imgui->font.view,
                .imageLayout = imgui->font.layout,
            },
        };
        const VkWriteDescriptorSet write_descs[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = imgui->set,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = NELEM(desc_images),
                .pImageInfo = desc_images,
            },
        };
        vkUpdateDescriptorSets(g_vkr.dev, NELEM(write_descs), write_descs, 0, NULL);
    }

    // Create Pipeline Layout:
    {
        const VkPushConstantRange ranges[] =
        {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .size = sizeof(float) * 4,
            },
        };
        const VkDescriptorSetLayout setLayouts[] = { imgui->setLayout };
        imgui->layout = vkrPipelineLayout_New(
            NELEM(setLayouts), setLayouts,
            NELEM(ranges), ranges);
    }

    const VkPipelineShaderStageCreateInfo stages[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main",
        },
    };
    const VkVertexInputBindingDescription binding_descs[] =
    {
        {
            .stride = sizeof(ImDrawVert),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    const VkVertexInputAttributeDescription attribute_descs[] =
    {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = pim_offsetof(ImDrawVert, pos),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = pim_offsetof(ImDrawVert, uv),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .offset = pim_offsetof(ImDrawVert, col),
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = NELEM(binding_descs),
        .pVertexBindingDescriptions = binding_descs,
        .vertexAttributeDescriptionCount = NELEM(attribute_descs),
        .pVertexAttributeDescriptions = attribute_descs,
    };

    const VkPipelineInputAssemblyStateCreateInfo ia_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    const VkPipelineViewportStateCreateInfo viewport_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo raster_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo ms_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState color_attachments[] =
    {
        {
            .blendEnable = true,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        },
        {
            .blendEnable = false,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = 0x0,
        },
    };

    const VkPipelineDepthStencilStateCreateInfo depth_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    const VkPipelineColorBlendStateCreateInfo blend_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = NELEM(color_attachments),
        .pAttachments = color_attachments,
    };

    const VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    const VkPipelineDynamicStateCreateInfo dynamic_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = NELEM(dynamic_states),
        .pDynamicStates = dynamic_states,
    };

    const VkGraphicsPipelineCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = 0x0,
        .stageCount = NELEM(stages),
        .pStages = stages,
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &ia_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_info,
        .pMultisampleState = &ms_info,
        .pDepthStencilState = &depth_info,
        .pColorBlendState = &blend_info,
        .pDynamicState = &dynamic_state,
        .layout = imgui->layout,
        .renderPass = renderPass,
    };
    VkCheck(vkCreateGraphicsPipelines(g_vkr.dev, NULL, 1, &info, NULL, &imgui->pipeline));

    vkDestroyShaderModule(g_vkr.dev, vert_module, NULL);
    vkDestroyShaderModule(g_vkr.dev, frag_module, NULL);

    return imgui->pipeline != NULL;
}

void vkrImGuiPass_Del(vkrImGui* imgui)
{
    if (imgui)
    {
        ASSERT(g_vkr.dev);
        vkDeviceWaitIdle(g_vkr.dev);

        vkrTexture2D_Del(&imgui->font);
        for (i32 i = 0; i < NELEM(imgui->vertbufs); ++i)
        {
            vkrBuffer_Del(&imgui->vertbufs[i]);
            vkrBuffer_Del(&imgui->indbufs[i]);
        }
        vkrDescPool_Del(imgui->descPool);
        vkrSetLayout_Del(imgui->setLayout);
        vkrPipelineLayout_Del(imgui->layout);
        vkrPipeline_Del(imgui->pipeline);

        memset(imgui, 0, sizeof(*imgui));
    }
}

ProfileMark(pm_igrender, igRender)
ProfileMark(pm_draw, vkrImGuiPass_Draw)
void vkrImGuiPass_Draw(
    vkrImGui* imgui,
    VkCommandBuffer cmd)
{
    ProfileBegin(pm_igrender);
    igRender();
    ProfileEnd(pm_igrender);

    ProfileBegin(pm_draw);
    vkrImGui_RenderDrawData(imgui, igGetDrawData(), cmd);
    ProfileEnd(pm_draw);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_setuprenderstate, vkrImGui_SetupRenderState)
static void vkrImGui_SetupRenderState(
    vkrImGui* imgui,
    const ImDrawData* draw_data,
    VkCommandBuffer cmd,
    i32 fb_width,
    i32 fb_height)
{
    ProfileBegin(pm_setuprenderstate);

    const u32 syncIndex = g_vkr.chain.syncIndex;
    // Bind pipeline and descriptor sets:
    {
        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            imgui->pipeline);
        const VkDescriptorSet descSets[] = { imgui->set };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            imgui->layout,
            0, NELEM(descSets), descSets,
            0, NULL);
    }

    // Bind Vertex And Index Buffer:
    if (draw_data->TotalVtxCount > 0)
    {
        const VkBuffer vbufs[] = { imgui->vertbufs[syncIndex].handle };
        const VkDeviceSize voffsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd,
            0, NELEM(vbufs), vbufs, voffsets);
        vkCmdBindIndexBuffer(
            cmd,
            imgui->indbufs[syncIndex].handle,
            0,
            sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    // Setup viewport:
    {
        const VkViewport viewport =
        {
            .x = 0,
            .y = 0,
            .width = (float)fb_width,
            .height = (float)fb_height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        const float constants[] =
        {
            2.0f / draw_data->DisplaySize.x,
            2.0f / draw_data->DisplaySize.y,
            -1.0f - draw_data->DisplayPos.x * (2.0f / draw_data->DisplaySize.x),
            -1.0f - draw_data->DisplayPos.y * (2.0f / draw_data->DisplaySize.y),
        };
        vkCmdPushConstants(
            cmd,
            imgui->layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(constants), constants);
    }
    ProfileEnd(pm_setuprenderstate);
}

ProfileMark(pm_upload, vkrImGui_UploadRenderDrawData)
static void vkrImGui_UploadRenderDrawData(vkrImGui* imgui, ImDrawData* draw_data)
{
    ProfileBegin(pm_upload);

    const u32 syncIndex = g_vkr.chain.syncIndex;
    vkrBuffer* const vertBuf = &imgui->vertbufs[syncIndex];
    vkrBuffer* const indBuf = &imgui->indbufs[syncIndex];

    const i32 totalVtxCount = draw_data->TotalVtxCount;
    const i32 totalIdxCount = draw_data->TotalIdxCount;
    const i32 vertex_size = totalVtxCount * sizeof(ImDrawVert);
    const i32 index_size = totalIdxCount * sizeof(ImDrawIdx);

    vkrBuffer_Reserve(
        vertBuf,
        vertex_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu,
        NULL,
        PIM_FILELINE);
    vkrBuffer_Reserve(
        indBuf,
        index_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu,
        NULL,
        PIM_FILELINE);

    ImDrawVert* vtx_dst = vkrBuffer_Map(vertBuf);
    ImDrawIdx* idx_dst = vkrBuffer_Map(indBuf);
    ASSERT(vtx_dst);
    ASSERT(idx_dst);
    i32 vertOffset = 0;
    i32 indOffset = 0;

    const i32 cmdListCount = draw_data->CmdListsCount;
    ImDrawList const *const *const pim_noalias cmdLists = draw_data->CmdLists;
    for (i32 n = 0; n < cmdListCount; n++)
    {
        const ImDrawList* cmdlist = cmdLists[n];
        const ImDrawVert* pVertSrc = cmdlist->VtxBuffer.Data;
        const ImDrawIdx* pIdxSrc = cmdlist->IdxBuffer.Data;
        i32 vlen = cmdlist->VtxBuffer.Size;
        i32 ilen = cmdlist->IdxBuffer.Size;
        memcpy(vtx_dst + vertOffset, pVertSrc, vlen * sizeof(ImDrawVert));
        memcpy(idx_dst + indOffset, pIdxSrc, ilen * sizeof(ImDrawIdx));
        vertOffset += vlen;
        indOffset += ilen;
    }

    vkrBuffer_Unmap(vertBuf);
    vkrBuffer_Unmap(indBuf);
    vkrBuffer_Flush(vertBuf);
    vkrBuffer_Flush(indBuf);

    ProfileEnd(pm_upload);
}

ProfileMark(pm_render, vkrImGui_RenderDrawData)
static void vkrImGui_RenderDrawData(
    vkrImGui* imgui,
    ImDrawData* draw_data,
    VkCommandBuffer command_buffer)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    const i32 fb_width = (i32)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const i32 fb_height = (i32)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
    {
        return;
    }

    const i32 cmdListCount = draw_data->CmdListsCount;
    ImDrawList const *const *const pim_noalias cmdLists = draw_data->CmdLists;
    const i32 totalVtxCount = draw_data->TotalVtxCount;
    const i32 totalIdxCount = draw_data->TotalIdxCount;
    if (totalVtxCount <= 0 || totalIdxCount <= 0)
    {
        return;
    }

    vkrImGui_UploadRenderDrawData(imgui, draw_data);
    vkrImGui_SetupRenderState(imgui, draw_data, command_buffer, fb_width, fb_height);

    ProfileBegin(pm_render);

    // Will project scissor/clipping rectangles into framebuffer space
    const ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    i32 global_vtx_offset = 0;
    i32 global_idx_offset = 0;
    for (i32 iList = 0; iList < cmdListCount; iList++)
    {
        ImDrawList const *const pim_noalias cmdList = cmdLists[iList];
        const i32 cmdCount = cmdList->CmdBuffer.Size;
        ImDrawCmd const *const pim_noalias cmds = cmdList->CmdBuffer.Data;
        for (i32 iCmd = 0; iCmd < cmdCount; iCmd++)
        {
            ImDrawCmd const *const pim_noalias pcmd = cmds + iCmd;
            ImDrawCallback userCallback = pcmd->UserCallback;
            if (userCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                const ImDrawCallback kResetRenderState = (ImDrawCallback)-1;
                if (userCallback == kResetRenderState)
                {
                    vkrImGui_SetupRenderState(
                        imgui,
                        draw_data,
                        command_buffer,
                        fb_width,
                        fb_height);
                }
                else
                {
                    userCallback(cmdList, pcmd);
                }
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect =
                {
                    .x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                    .y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y,
                    .z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                    .w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y,
                };

                if ((clip_rect.x < fb_width) &&
                    (clip_rect.y < fb_height) &&
                    (clip_rect.z > 0.0f) &&
                    (clip_rect.w > 0.0f))
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    clip_rect.x = clip_rect.x < 0.0f ? 0.0f : clip_rect.x;
                    clip_rect.y = clip_rect.y < 0.0f ? 0.0f : clip_rect.y;

                    // Apply scissor/clipping rectangle
                    const VkRect2D scissor =
                    {
                        .offset.x = (i32)clip_rect.x,
                        .offset.y = (i32)clip_rect.y,
                        .extent.width = (u32)(clip_rect.z - clip_rect.x),
                        .extent.height = (u32)(clip_rect.w - clip_rect.y),
                    };
                    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                    // Draw
                    vkCmdDrawIndexed(
                        command_buffer,
                        pcmd->ElemCount,
                        1,
                        pcmd->IdxOffset + global_idx_offset,
                        pcmd->VtxOffset + global_vtx_offset,
                        0);
                }
            }
        }
        global_idx_offset += cmdList->IdxBuffer.Size;
        global_vtx_offset += cmdList->VtxBuffer.Size;
    }

    ProfileEnd(pm_render);
}
