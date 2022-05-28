#include "rendering/vulkan/vkr_uipass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_framebuffer.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/texture.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "ui/cimgui_ext.h"
#include <string.h>

typedef struct PushConstants_s
{
    float2 scale;
    float2 translate;

    u32 textureIndex;
    u32 discardAlpha;
} PushConstants;

static VkRenderPass ms_renderPass;
static vkrPass ms_pass;
static vkrBufferSet ms_vertbufs;
static vkrBufferSet ms_indbufs;
static vkrTextureId ms_font;

static void vkrImGui_SetupRenderState(
    vkrCmdBuf* command_buffer,
    i32 fb_width,
    i32 fb_height);
static void vkrImGui_UploadRenderDrawData(vkrCmdBuf* cmd);
static void vkrImGui_RenderDrawData(vkrCmdBuf* cmd);
static void vkrImGui_SetTexture(vkrCmdBuf* cmd, vkrTextureId id);
static vkrTextureId ToVkrTextureId(ImTextureID imid);
static ImTextureID ToImTextureId(vkrTextureId vkrid);

// ----------------------------------------------------------------------------

bool vkrUIPass_New(void)
{
    bool success = true;

    // Setup back-end capabilities flags
    {
        ImGuiIO* io = igGetIO();
        ASSERT(io);
        io->BackendRendererName = "vkrImGui";
        io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    }

    const vkrImage* backBuffer = vkrGetBackBuffer();
    const vkrRenderPassDesc renderPassDesc =
    {
        .srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

        .dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

        .attachments[0] =
        {
            .format = backBuffer->format,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_LOAD,
            .store = VK_ATTACHMENT_STORE_OP_STORE,
        },
    };
    ms_renderPass = vkrRenderPass_Get(&renderPassDesc);
    if (!ms_renderPass)
    {
        success = false;
        goto cleanup;
    }

    // Create Mesh Buffers:
    if (!vkrBufferSet_New(&ms_vertbufs, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vkrMemUsage_Dynamic))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrBufferSet_New(&ms_indbufs, 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, vkrMemUsage_Dynamic))
    {
        success = false;
        goto cleanup;
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

        ms_font = vkrTexTable_Alloc(
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            width, height, 1, 1, true);
        vkrTexTable_Upload(ms_font, 0, pixels, sizeof(u32) * width * height);
        SASSERT(sizeof(io->Fonts[0].TexID) >= sizeof(ms_font));
        memcpy(&(io->Fonts[0].TexID), &ms_font, sizeof(ms_font));
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
        .pushConstantBytes = sizeof(PushConstants),
        .shaderCount = NELEM(shaders),
        .shaders = shaders,
        .renderPass = ms_renderPass,
        .subpass = 0,
        .vertLayout =
        {
            .bindingCount = NELEM(vertBindings),
            .bindings = vertBindings,
            .attributeCount = NELEM(vertAttributes),
            .attributes = vertAttributes,
        },
        .fixedFuncs =
        {
            .viewport = vkrSwapchain_GetViewport(),
            .scissor = vkrSwapchain_GetRect(),
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

    if (!vkrPass_New(&ms_pass, &desc))
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
        vkrUIPass_Del();
    }

    return success;
}

void vkrUIPass_Del(void)
{
    vkrTexTable_Free(ms_font);
    vkrBufferSet_Release(&ms_vertbufs);
    vkrBufferSet_Release(&ms_indbufs);
    vkrPass_Del(&ms_pass);
}

void vkrUIPass_Setup(void)
{
}

ProfileMark(pm_draw, vkrUIPass_Execute)
void vkrUIPass_Execute(void)
{
    ProfileBegin(pm_draw);
    igRender();

    vkrCmdBuf* cmd = vkrCmdGet_G();
    vkrImGui_UploadRenderDrawData(cmd);
    vkrCmdBindPass(cmd, &ms_pass);

    const VkClearValue clearValues[] =
    {
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
    };
    vkrImage* attachments[] = { vkrGetBackBuffer() };
    VkRect2D rect = { 0 };
    rect.extent.width = attachments[0]->width;
    rect.extent.height = attachments[0]->height;
    VkFramebuffer framebuffer = vkrFramebuffer_Get(attachments, NELEM(attachments), rect.extent.width, rect.extent.height);
    vkrImageState_ColorAttachWrite(cmd, attachments[0]);
    vkrCmdBeginRenderPass(
        cmd,
        ms_renderPass,
        framebuffer,
        rect,
        NELEM(clearValues), clearValues);

    vkrImGui_RenderDrawData(cmd);

    vkrCmdEndRenderPass(cmd);

    ProfileEnd(pm_draw);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_setuprenderstate, vkrImGui_SetupRenderState)
static void vkrImGui_SetupRenderState(
    vkrCmdBuf* cmd,
    i32 fb_width,
    i32 fb_height)
{
    ProfileBegin(pm_setuprenderstate);

    const VkViewport viewport =
    {
        .x = 0,
        .y = 0,
        .width = (float)fb_width,
        .height = (float)fb_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd->handle, 0, 1, &viewport);
    vkrImGui_SetTexture(cmd, ms_font);

    ProfileEnd(pm_setuprenderstate);
}

ProfileMark(pm_upload, vkrImGui_UploadRenderDrawData)
static void vkrImGui_UploadRenderDrawData(vkrCmdBuf* cmd)
{
    ProfileBegin(pm_upload);

    const ImDrawData * const drawData = igGetDrawData();
    if (drawData->TotalVtxCount > 0)
    {
        const i32 totalVtxCount = drawData->TotalVtxCount;
        const i32 totalIdxCount = drawData->TotalIdxCount;
        const i32 vertex_size = totalVtxCount * sizeof(ImDrawVert);
        const i32 index_size = totalIdxCount * sizeof(ImDrawIdx);

        vkrBuffer* const vertBuf = vkrBufferSet_Current(&ms_vertbufs);
        vkrBuffer* const indBuf = vkrBufferSet_Current(&ms_indbufs);
        vkrBuffer_Reserve(
            vertBuf,
            vertex_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            vkrMemUsage_Dynamic);
        vkrBuffer_Reserve(
            indBuf,
            index_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            vkrMemUsage_Dynamic);

        ImDrawVert* pim_noalias vtx_dst = vkrBuffer_MapWrite(vertBuf);
        ImDrawIdx* pim_noalias idx_dst = vkrBuffer_MapWrite(indBuf);
        ASSERT(vtx_dst);
        ASSERT(idx_dst);
        i32 vertOffset = 0;
        i32 indOffset = 0;

        const i32 cmdListCount = drawData->CmdListsCount;
        ImDrawList const *const *const pim_noalias cmdLists = drawData->CmdLists;
        for (i32 iCmdList = 0; iCmdList < cmdListCount; iCmdList++)
        {
            const ImDrawList* pim_noalias cmdlist = cmdLists[iCmdList];
            i32 vlen = cmdlist->VtxBuffer.Size;
            i32 ilen = cmdlist->IdxBuffer.Size;
            memcpy(vtx_dst + vertOffset, cmdlist->VtxBuffer.Data, vlen * sizeof(ImDrawVert));
            memcpy(idx_dst + indOffset, cmdlist->IdxBuffer.Data, ilen * sizeof(ImDrawIdx));
            vertOffset += vlen;
            indOffset += ilen;
        }

        vkrBuffer_UnmapWrite(vertBuf);
        vkrBuffer_UnmapWrite(indBuf);

        vkrBufferState_VertexBuffer(cmd, vertBuf);
        vkrBufferState_IndexBuffer(cmd, indBuf);
        const VkBuffer vbufs[] = { vertBuf->handle };
        const VkDeviceSize voffsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd->handle,
            0, NELEM(vbufs), vbufs, voffsets);
        vkCmdBindIndexBuffer(
            cmd->handle,
            indBuf->handle,
            0,
            sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    ProfileEnd(pm_upload);
}

static vkrTextureId ToVkrTextureId(ImTextureID imid)
{
    vkrTextureId vkrid;
    SASSERT(sizeof(imid) >= sizeof(vkrid));
    memcpy(&vkrid, &imid, sizeof(vkrid));
    return vkrid;
}

static ImTextureID ToImTextureId(vkrTextureId vkrid)
{
    ImTextureID imid = { 0 };
    SASSERT(sizeof(imid) >= sizeof(vkrid));
    memcpy(&imid, &vkrid, sizeof(vkrid));
    return imid;
}

static void vkrImGui_SetTexture(
    vkrCmdBuf* cmd,
    vkrTextureId id)
{
    vkrTextureId font = ms_font;
    u32 discardAlpha = 0;
    u32 index = 0;
    if (vkrTexTable_Exists(id))
    {
        index = id.index;
        discardAlpha = memcmp(&id, &font, sizeof(id)) != 0;
    }

    const ImDrawData* drawData = igGetDrawData();
    const float2 sc = { 2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y };
    const float2 tr = { -1.0f - drawData->DisplayPos.x * sc.x, -1.0f - drawData->DisplayPos.y * sc.y };

    const PushConstants constants =
    {
        .scale = sc,
        .translate = tr,
        .textureIndex = index,
        .discardAlpha = discardAlpha,
    };
    vkrCmdPushConstants(cmd, &ms_pass, &constants, sizeof(constants));
}

ProfileMark(pm_render, vkrImGui_RenderDrawData)
static void vkrImGui_RenderDrawData(vkrCmdBuf* cmd)
{
    ProfileBegin(pm_render);

    const ImDrawData* drawData = igGetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    const i32 fb_width = (i32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    const i32 fb_height = (i32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
    {
        goto end;
    }

    const i32 cmdListCount = drawData->CmdListsCount;
    ImDrawList const *const *const pim_noalias cmdLists = drawData->CmdLists;
    const i32 totalVtxCount = drawData->TotalVtxCount;
    const i32 totalIdxCount = drawData->TotalIdxCount;
    if (totalVtxCount <= 0 || totalIdxCount <= 0)
    {
        goto end;
    }

    vkrImGui_SetupRenderState(cmd, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    const ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    const ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

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
                        cmd,
                        fb_width,
                        fb_height);
                }
                else
                {
                    vkrImGui_SetTexture(cmd, ToVkrTextureId(pcmd->TextureId));
                    userCallback(cmdList, pcmd);
                }
            }
            else
            {
                vkrImGui_SetTexture(cmd, ToVkrTextureId(pcmd->TextureId));
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
                    vkCmdSetScissor(cmd->handle, 0, 1, &scissor);

                    // Draw
                    vkCmdDrawIndexed(
                        cmd->handle,
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

end:
    ProfileEnd(pm_render);
}
