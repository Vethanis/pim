#include "rendering/vulkan/vkr_imgui.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "ui/cimgui.h"
#include <string.h>

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

    VkPipelineShaderStageCreateInfo stages[2] = {0};
    if (!vkrShader_New(&stages[0], "imgui.hlsl", "VSMain", vkrShaderType_Vert))
    {
        return false;
    }
    if (!vkrShader_New(&stages[1], "imgui.hlsl", "PSMain", vkrShaderType_Frag))
    {
        return false;
    }
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
            .blendEnable = true,
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

    vkrShader_Del(&stages[0]);
    vkrShader_Del(&stages[1]);

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
