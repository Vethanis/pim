#include "screenblit.h"
#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/constants.h"
#include "common/console.h"
#include "math/types.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

static const float2 kScreenMesh[] =
{
    { -1.0f, -1.0f },
    { 1.0f, -1.0f },
    { 1.0f,  1.0f },
    { 1.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
};

static vkrBuffer ms_blitMesh;
static vkrPipeline ms_blitProgram;
static vkrTexture2D ms_image;

// ----------------------------------------------------------------------------

bool screenblit_init(
    VkRenderPass renderPass,
    i32 subpass)
{
    bool success = true;

    vkrCompileOutput vertOutput = { 0 };
    vkrCompileOutput fragOutput = { 0 };
    char* shaderText = vkrLoadShader("screenblit.hlsl");

    if (!vkrTexture2D_New(
        &ms_image,
        kDrawWidth,
        kDrawHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        NULL, 0))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &ms_blitMesh,
        sizeof(kScreenMesh),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu,
        PIM_FILELINE))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    const vkrCompileInput vertInput =
    {
        .filename = "screenblit.hlsl",
        .entrypoint = "VSMain",
        .text = shaderText,
        .type = vkrShaderType_Vert,
        .compile = true,
        .disassemble = true,
    };
    vkrCompile(&vertInput, &vertOutput);
    if (vertOutput.errors)
    {
        con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", vertInput.filename, vertInput.entrypoint);
        con_logf(LogSev_Error, "Vkc", "%s", vertOutput.errors);
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (vertOutput.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", vertInput.filename, vertInput.entrypoint);
        con_logf(LogSev_Info, "Vkc", "%s", vertOutput.disassembly);
    }

    const vkrCompileInput fragInput =
    {
        .filename = "screenblit.hlsl",
        .entrypoint = "PSMain",
        .text = shaderText,
        .type = vkrShaderType_Frag,
        .compile = true,
        .disassemble = true,
    };
    vkrCompile(&fragInput, &fragOutput);
    if (fragOutput.errors)
    {
        con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", fragInput.filename, fragInput.entrypoint);
        con_logf(LogSev_Error, "Vkc", "%s", fragOutput.errors);
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (fragOutput.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", fragInput.filename, fragInput.entrypoint);
        con_logf(LogSev_Info, "Vkc", "%s", fragOutput.disassembly);
    }

    const vkrSwapchain* swapchain = &g_vkr.chain;
    ASSERT(swapchain->handle);

    const vkrFixedFuncs ffuncs =
    {
        .viewport = vkrSwapchain_GetViewport(swapchain),
        .scissor = vkrSwapchain_GetRect(swapchain),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .cullMode = VK_CULL_MODE_NONE,
        .depthCompareOp = VK_COMPARE_OP_ALWAYS,
        .scissorOn = false,
        .depthClamp = false,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .attachmentCount = 1,
        .attachments[0] =
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = false,
        },
    };

    const vkrVertexLayout vertLayout =
    {
        .streamCount = 1,
        .types[0] = vkrVertType_float2,
    };

    VkPipelineShaderStageCreateInfo shaders[] =
    {
        vkrCreateShader(&vertOutput),
        vkrCreateShader(&fragOutput),
    };

    vkrPipelineLayout pipeLayout = { 0 };
    vkrPipelineLayout_New(&pipeLayout);
    const VkDescriptorSetLayoutBinding bindings[] =
    {
        {
            // texture + sampler
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    if (!vkrPipelineLayout_AddSet(
        &pipeLayout,
        NELEM(bindings), bindings,
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrPipeline_NewGfx(
        &ms_blitProgram,
        &ffuncs,
        &vertLayout,
        &pipeLayout,
        renderPass,
        subpass,
        NELEM(shaders), shaders))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    vkrCompileOutput_Del(&vertOutput);
    vkrCompileOutput_Del(&fragOutput);
    pim_free(shaderText);
    shaderText = NULL;
    for (i32 i = 0; i < NELEM(shaders); ++i)
    {
        vkrDestroyShader(shaders + i);
    }
    if (!success)
    {
        screenblit_shutdown();
    }
    return success;
}

void screenblit_shutdown(void)
{
    vkrTexture2D_Del(&ms_image);
    vkrBuffer_Release(&ms_blitMesh, NULL);
    vkrPipeline_Del(&ms_blitProgram);
}

ProfileMark(pm_blit, screenblit_blit)
VkCommandBuffer screenblit_blit(
    const u32* texels,
    i32 width,
    i32 height,
    VkRenderPass renderPass)
{
    ProfileBegin(pm_blit);

    ASSERT(renderPass);
    ASSERT(ms_image.width == width);
    ASSERT(ms_image.height == height);
    const i32 bytes = width * height * sizeof(texels[0]);
    vkrTexture2D_Upload(&ms_image, texels, bytes);

    const vkrSwapchain* swapchain = &g_vkr.chain;
    VkViewport viewport = vkrSwapchain_GetViewport(swapchain);
    VkRect2D rect = vkrSwapchain_GetRect(swapchain);

    vkrFrameContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = NULL;
    vkrContext_GetSecCmd(ctx, vkrQueueId_Gfx, &cmd, NULL, NULL);
    vkrCmdBeginSec(cmd, renderPass, 0, NULL);
    vkrCmdViewport(cmd, viewport, rect);
    vkrCmdBindPipeline(cmd, &ms_blitProgram);
    const VkDescriptorImageInfo imgInfo =
    {
        .sampler = ms_image.sampler,
        .imageView = ms_image.view,
        .imageLayout = ms_image.layout,
    };
    const VkWriteDescriptorSet writes[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imgInfo,
        },
    };
    vkCmdPushDescriptorSetKHR(
        cmd,
        ms_blitProgram.bindpoint,
        ms_blitProgram.layout.handle,
        0,
        NELEM(writes), writes);
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &ms_blitMesh.handle, offsets);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    vkrCmdEnd(cmd);

    ProfileEnd(pm_blit);

    return cmd;
}
