#include "rendering/vulkan/vkr_exposurepass.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_bindings.h"

#include "rendering/exposure.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

bool vkrExposurePass_New(vkrExposurePass *const pass)
{
    ASSERT(pass);
    memset(pass, 0, sizeof(*pass));
    bool success = true;

    VkPipelineShaderStageCreateInfo buildShaders[1] = { 0 };
    VkPipelineShaderStageCreateInfo adaptShaders[1] = { 0 };
    if (!vkrShader_New(&buildShaders[0], "BuildHistogram.hlsl", "CSMain", vkrShaderType_Comp))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrShader_New(&adaptShaders[0], "AdaptHistogram.hlsl", "CSMain", vkrShaderType_Comp))
    {
        success = false;
        goto cleanup;
    }
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        const i32 bytes = sizeof(u32) * kHistogramSize;
        if (!vkrBuffer_New(
            &pass->histBuffers[i],
            bytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            vkrMemUsage_CpuToGpu,
            PIM_FILELINE))
        {
            success = false;
            goto cleanup;
        }
    }
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        const i32 bytes = sizeof(float) * 2;
        if (!vkrBuffer_New(
            &pass->expBuffers[i],
            bytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            vkrMemUsage_CpuToGpu,
            PIM_FILELINE))
        {
            success = false;
            goto cleanup;
        }
    }
    const vkrPassDesc desc =
    {
        .pushConstantBytes = sizeof(vkrExposureConstants),
        .shaderCount = NELEM(buildShaders),
        .shaders = buildShaders,
    };
    if (!vkrPass_New(&pass->pass, &desc))
    {
        success = false;
        goto cleanup;
    }
    pass->adapt = vkrPipeline_NewComp(pass->pass.layout, &adaptShaders[0]);
    if (!pass->adapt)
    {
        success = false;
        goto cleanup;
    }

cleanup:
    for (i32 i = 0; i < NELEM(buildShaders); ++i)
    {
        vkrShader_Del(&buildShaders[i]);
    }
    for (i32 i = 0; i < NELEM(adaptShaders); ++i)
    {
        vkrShader_Del(&adaptShaders[i]);
    }
    if (!success)
    {
        vkrExposurePass_Del(pass);
    }
    return success;
}

void vkrExposurePass_Del(vkrExposurePass *const pass)
{
    if (pass)
    {
        vkrPipeline_Del(pass->adapt);
        vkrPass_Del(&pass->pass);
        for (i32 i = 0; i < NELEM(pass->histBuffers); ++i)
        {
            vkrBuffer_Del(&pass->histBuffers[i]);
        }
        for (i32 i = 0; i < NELEM(pass->expBuffers); ++i)
        {
            vkrBuffer_Del(&pass->expBuffers[i]);
        }
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_setup, vkrExposurePass_Setup)
void vkrExposurePass_Setup(vkrExposurePass *const pass)
{
    ProfileBegin(pm_setup);

    pass->params.deltaTime = f1_lerp(pass->params.deltaTime, (float)time_dtf(), 0.25f);

    vkrSwapchain *const chain = &g_vkr.chain;
    const u32 chainLen = chain->length;
    const u32 imgIndex = (chain->imageIndex + (chainLen - 1u)) % chainLen;
    const u32 syncIndex = (chain->syncIndex + (kFramesInFlight - 1u)) % kFramesInFlight;

    vkrAttachment *const lum = &chain->lumAttachments[imgIndex];
    vkrBuffer *const expBuffer = &pass->expBuffers[syncIndex];
    vkrBuffer *const histBuffer = &pass->histBuffers[syncIndex];

    vkrBindings_BindImage(
        vkrBindId_LumTexture,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        NULL,
        lum->view,
        VK_IMAGE_LAYOUT_GENERAL);
    vkrBindings_BindBuffer(
        vkrBindId_ExposureBuffer,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        expBuffer);
    vkrBindings_BindBuffer(
        vkrBindId_HistogramBuffer,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        histBuffer);

    ProfileEnd(pm_setup);
}

ProfileMark(pm_execute, vkrExposurePass_Execute)
void vkrExposurePass_Execute(vkrExposurePass *const pass)
{
    ProfileBegin(pm_execute);

    vkrSwapchain *const chain = &g_vkr.chain;
    const u32 chainLen = chain->length;
    const u32 imgIndex = (chain->imageIndex + (chainLen - 1u)) % chainLen;
    const u32 syncIndex = (chain->syncIndex + (kFramesInFlight - 1u)) % kFramesInFlight;

    vkrAttachment *const lum = &chain->lumAttachments[imgIndex];
    vkrBuffer *const expBuffer = &pass->expBuffers[syncIndex];
    vkrBuffer *const histBuffer = &pass->histBuffers[syncIndex];

    VkDescriptorSet set = vkrBindings_GetSet();
    VkPipelineLayout layout = pass->pass.layout;
    VkPipeline buildPipe = pass->pass.pipeline;
    VkPipeline adaptPipe = pass->adapt;

    vkrThreadContext *const ctx = vkrContext_Get();

    VkFence cmpfence = NULL;
    VkQueue cmpqueue = NULL;
    VkCommandBuffer cmpcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &cmpfence, &cmpqueue);
    vkrCmdBegin(cmpcmd);

    // transition lum img and exposure buf to compute
    {
        VkFence gfxfence = NULL;
        VkQueue gfxqueue = NULL;
        VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &gfxfence, &gfxqueue);
        vkrCmdBegin(gfxcmd);

        vkrImage_Transfer(
            &lum->image,
            vkrQueueId_Gfx,
            vkrQueueId_Comp,
            gfxcmd,
            cmpcmd,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkrBuffer_Transfer(
            expBuffer,
            vkrQueueId_Gfx,
            vkrQueueId_Comp,
            gfxcmd,
            cmpcmd,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        vkrCmdEnd(gfxcmd);
        vkrCmdSubmit(gfxqueue, gfxcmd, gfxfence, NULL, 0x0, NULL);
    }

    // dispatch shaders
    {
        vkCmdBindDescriptorSets(
            cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);

        const i32 width = chain->width;
        const i32 height = chain->height;
        const vkrExposureConstants constants =
        {
            .width = width,
            .height = height,
            .exposure = pass->params,
        };
        vkCmdPushConstants(
            cmpcmd,
            layout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(vkrExposureConstants),
            &constants);

        // build histogram
        {
            const i32 dispatchX = (width + 15) / 16;
            const i32 dispatchY = (height + 15) / 16;
            vkCmdBindPipeline(cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, buildPipe);
            vkCmdDispatch(cmpcmd, dispatchX, dispatchY, 1);
        }

        vkrBuffer_Barrier(
            histBuffer,
            cmpcmd,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        // adapt exposure
        {
            vkCmdBindPipeline(cmpcmd, VK_PIPELINE_BIND_POINT_COMPUTE, adaptPipe);
            vkCmdDispatch(cmpcmd, 1, 1, 1);
        }

    }

    // transition to gfx
    {
        VkFence gfxfence = NULL;
        VkQueue gfxqueue = NULL;
        VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &gfxfence, &gfxqueue);
        vkrCmdBegin(gfxcmd);

        vkrImage_Transfer(
            &lum->image,
            vkrQueueId_Comp,
            vkrQueueId_Gfx,
            cmpcmd,
            gfxcmd,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        vkrBuffer_Transfer(
            expBuffer,
            vkrQueueId_Comp,
            vkrQueueId_Gfx,
            cmpcmd,
            gfxcmd,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkrCmdEnd(cmpcmd);
        vkrCmdSubmit(cmpqueue, cmpcmd, cmpfence, NULL, 0x0, NULL);

        vkrCmdEnd(gfxcmd);
        vkrCmdSubmit(gfxqueue, gfxcmd, gfxfence, NULL, 0x0, NULL);
    }

    ProfileEnd(pm_execute);
}
