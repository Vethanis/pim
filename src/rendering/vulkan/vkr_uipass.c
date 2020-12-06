#include "rendering/vulkan/vkr_uipass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "ui/cimgui.h"
#include <string.h>

// ----------------------------------------------------------------------------

static void vkrImGui_SetupRenderState(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data,
    VkCommandBuffer command_buffer,
    i32 fb_width,
    i32 fb_height);
static void vkrImGui_UploadRenderDrawData(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data);
static void vkrImGui_RenderDrawData(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data,
    VkCommandBuffer command_buffer);
static void vkrImGui_SetTexture(
    vkrUIPass const *const imgui,
    VkCommandBuffer cmd,
    ImDrawData const *const draw_data,
    vkrTextureId id);

// ----------------------------------------------------------------------------

bool vkrUIPass_New(vkrUIPass *const imgui, VkRenderPass renderPass)
{
    ASSERT(imgui);
    ASSERT(renderPass);
    memset(imgui, 0, sizeof(*imgui));

    bool success = true;

    // Setup back-end capabilities flags
    {
        ImGuiIO* io = igGetIO();
        ASSERT(io);
        io->BackendRendererName = "vkrImGui";
        io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    }

    // Create Mesh Buffers:
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        if (!vkrBuffer_New(
            &imgui->vertbufs[i],
            1024,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            vkrMemUsage_CpuToGpu))
        {
            success = false;
            goto cleanup;
        }
        if (!vkrBuffer_New(
            &imgui->indbufs[i],
            1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            vkrMemUsage_CpuToGpu))
        {
            success = false;
            goto cleanup;
        }
    }

    // Create Font Texture:
    {
        ImGuiIO* io = igGetIO();

        u8* pixels = NULL;
        i32 width = 0;
        i32 height = 0;
        i32 bpp = 0;
        ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, &bpp);

        ASSERT(pixels);
        vkrTextureId slot = vkrTexTable_Alloc(width, height, VK_FORMAT_R8G8B8A8_SRGB, pixels);
        imgui->font = slot;
        io->Fonts[0].TexID = *(u32*)&slot;
    }

    VkPipelineShaderStageCreateInfo shaders[2] = { 0 };
    if (!vkrShader_New(&shaders[0], "imgui.hlsl", "VSMain", vkrShaderType_Vert))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrShader_New(&shaders[1], "imgui.hlsl", "PSMain", vkrShaderType_Frag))
    {
        success = false;
        goto cleanup;
    }

    const VkDescriptorPoolSize poolSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kTextureDescriptors,
        },
    };

    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .size = sizeof(vkrUIPassPc),
        },
    };

    const VkVertexInputBindingDescription vertBindings[] =
    {
        {
            .stride = sizeof(ImDrawVert),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    const VkVertexInputAttributeDescription vertAttributes[] =
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

    const vkrPassDesc desc =
    {
        .pushConstantBytes = sizeof(vkrUIPassPc),
        .shaderCount = NELEM(shaders),
        .shaders = shaders,
        .renderPass = renderPass,
        .subpass = vkrPassId_UI,
        .vertLayout =
        {
            .bindingCount = NELEM(vertBindings),
            .bindings = vertBindings,
            .attributeCount = NELEM(vertAttributes),
            .attributes = vertAttributes,
        },
        .fixedFuncs =
        {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .scissorOn = true,
            .depthClamp = false,
            .depthTestEnable = false,
            .depthWriteEnable = false,
            .attachmentCount = 1,
            .attachments[0] =
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
        },
    };

    if (!vkrPass_New(&imgui->pass, &desc))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    for (i32 i = 0; i < NELEM(shaders); ++i)
    {
        vkrShader_Del(&shaders[i]);
    }
    if (!success)
    {
        vkrUIPass_Del(imgui);
    }

    return success;
}

void vkrUIPass_Del(vkrUIPass *const pass)
{
    if (pass)
    {
        vkrTexTable_Free(pass->font);
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Del(&pass->vertbufs[i]);
            vkrBuffer_Del(&pass->indbufs[i]);
        }
        vkrPass_Del(&pass->pass);
        memset(pass, 0, sizeof(*pass));
    }
}

void vkrUIPass_Setup(vkrUIPass *const pass)
{
}

ProfileMark(pm_draw, vkrUIPass_Execute)
void vkrUIPass_Execute(
    vkrPassContext const *const ctx,
    vkrUIPass *const pass)
{
    ProfileBegin(pm_draw);
    igRender();
    vkrImGui_RenderDrawData(pass, igGetDrawData(), ctx->cmd);
    ProfileEnd(pm_draw);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_setuprenderstate, vkrImGui_SetupRenderState)
static void vkrImGui_SetupRenderState(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data,
    VkCommandBuffer cmd,
    i32 fb_width,
    i32 fb_height)
{
    ProfileBegin(pm_setuprenderstate);

    const u32 syncIndex = vkr_syncIndex();
    VkDescriptorSet set = vkrBindings_GetSet();
    // Bind pipeline and descriptor sets:
    {
        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            imgui->pass.pipeline);
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            imgui->pass.layout,
            0, 1, &set,
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
    vkrImGui_SetTexture(imgui, cmd, draw_data, imgui->font);

    ProfileEnd(pm_setuprenderstate);
}

ProfileMark(pm_upload, vkrImGui_UploadRenderDrawData)
static void vkrImGui_UploadRenderDrawData(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data)
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
        NULL);
    vkrBuffer_Reserve(
        indBuf,
        index_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu,
        NULL);

    ImDrawVert *const vtx_dst = vkrBuffer_Map(vertBuf);
    ImDrawIdx *const idx_dst = vkrBuffer_Map(indBuf);
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

static void vkrImGui_SetTexture(
    vkrUIPass const *const imgui,
    VkCommandBuffer cmd,
    ImDrawData const *const draw_data,
    vkrTextureId id)
{
    vkrTextureId font = imgui->font;
    u32 discardAlpha = 0;
    u32 index = 0;
    if (vkrTexTable_Exists(id))
    {
        index = id.index;
        discardAlpha = id.asint != font.asint;
    }

    const float2 sc = { 2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y };
    const float2 tr = { -1.0f - draw_data->DisplayPos.x * sc.x, -1.0f - draw_data->DisplayPos.y * sc.y };

    const vkrUIPassPc constants =
    {
        .scale = sc,
        .translate = tr,
        .textureIndex = index,
        .discardAlpha = discardAlpha,
    };
    vkCmdPushConstants(
        cmd,
        imgui->pass.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(constants), &constants);
}

ProfileMark(pm_render, vkrImGui_RenderDrawData)
static void vkrImGui_RenderDrawData(
    vkrUIPass *const imgui,
    ImDrawData const *const draw_data,
    VkCommandBuffer cmd)
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
    vkrImGui_SetupRenderState(imgui, draw_data, cmd, fb_width, fb_height);

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
            ImDrawCmd const* const pim_noalias pcmd = cmds + iCmd;
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
                        cmd,
                        fb_width,
                        fb_height);
                }
                else
                {
                    vkrImGui_SetTexture(imgui, cmd, draw_data, *(vkrTextureId*)&pcmd->TextureId);
                    userCallback(cmdList, pcmd);
                }
            }
            else
            {
                vkrImGui_SetTexture(imgui, cmd, draw_data, *(vkrTextureId*)&pcmd->TextureId);
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
                    vkCmdSetScissor(cmd, 0, 1, &scissor);

                    // Draw
                    vkCmdDrawIndexed(
                        cmd,
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
