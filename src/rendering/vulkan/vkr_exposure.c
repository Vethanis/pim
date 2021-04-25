#include "rendering/vulkan/vkr_exposure.h"

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

typedef struct PushConstants_s
{
    u32 width;
    u32 height;
    vkrExposure exposure;
} PushConstants;

static vkrPass ms_buildPass;
static vkrPass ms_adaptPass;
static vkrBufferSet ms_histBuffers;
static vkrBufferSet ms_expBuffers;
static vkrBufferSet ms_readbackBuffers;
static float4 ms_readbackValue;
static VkFence ms_lastFence;
static vkrExposure ms_params;

bool vkrExposure_Init(void)
{
    bool success = true;

    VkPipelineShaderStageCreateInfo buildShaders[1] = { 0 };
    VkPipelineShaderStageCreateInfo adaptShaders[1] = { 0 };

    {
        if (!vkrShader_New(
            &buildShaders[0],
            "BuildHistogram.hlsl", "CSMain", vkrShaderType_Comp))
        {
            success = false;
            goto cleanup;
        }
        const vkrPassDesc desc =
        {
            .pushConstantBytes = sizeof(PushConstants),
            .shaderCount = NELEM(buildShaders),
            .shaders = buildShaders,
        };
        if (!vkrPass_New(&ms_buildPass, &desc))
        {
            success = false;
            goto cleanup;
        }
    }

    {
        if (!vkrShader_New(
            &adaptShaders[0],
            "AdaptHistogram.hlsl", "CSMain", vkrShaderType_Comp))
        {
            success = false;
            goto cleanup;
        }
        const vkrPassDesc desc =
        {
            .pushConstantBytes = sizeof(PushConstants),
            .shaderCount = NELEM(adaptShaders),
            .shaders = adaptShaders,
        };
        if (!vkrPass_New(&ms_adaptPass, &desc))
        {
            success = false;
            goto cleanup;
        }
    }

    if (!vkrBufferSet_New(
        &ms_histBuffers,
        sizeof(u32) * kHistogramSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        vkrMemUsage_GpuOnly))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrBufferSet_New(
        &ms_expBuffers,
        sizeof(float4),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_GpuOnly))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrBufferSet_New(
        &ms_readbackBuffers,
        ms_expBuffers.frames[0].size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        vkrMemUsage_CpuOnly))
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
        vkrExposure_Shutdown();
    }
    return success;
}

void vkrExposure_Shutdown(void)
{
    vkrPass_Del(&ms_buildPass);
    vkrPass_Del(&ms_adaptPass);
    vkrBufferSet_Release(&ms_histBuffers);
    vkrBufferSet_Release(&ms_expBuffers);
    vkrBufferSet_Release(&ms_readbackBuffers);
}

ProfileMark(pm_setup, vkrExposure_Setup)
void vkrExposure_Setup(void)
{
    ProfileBegin(pm_setup);

    ms_params.deltaTime = f1_lerp(ms_params.deltaTime, (float)Time_Deltaf(), 0.25f);

    vkrSwapchain *const chain = &g_vkr.chain;
    const u32 chainLen = chain->length;
    const u32 imgIndex = (chain->imageIndex + (chainLen - 1u)) % chainLen;
    const u32 syncIndex = (chain->syncIndex + (kResourceSets - 1u)) % kResourceSets;

    vkrImage *const lum = &chain->lumAttachments[imgIndex];
    vkrBuffer *const expBuffer = &ms_expBuffers.frames[syncIndex];
    vkrBuffer *const histBuffer = &ms_histBuffers.frames[syncIndex];

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

ProfileMark(pm_execute, vkrExposure_Execute)
ProfileMark(pm_readback, vkrExposure_Readback)
void vkrExposure_Execute(void)
{
    ProfileBegin(pm_readback);
    if (ms_lastFence)
    {
        vkrFence_Wait(ms_lastFence);
        ms_lastFence = NULL;
        vkrBufferSet_Read(
            &ms_readbackBuffers,
            &ms_readbackValue,
            sizeof(ms_readbackValue));
    }
    ProfileEnd(pm_readback);

    ProfileBegin(pm_execute);

    vkrSwapchain *const chain = &g_vkr.chain;
    const u32 chainLen = chain->length;
    const u32 imgIndex = (chain->imageIndex + (chainLen - 1u)) % chainLen;
    const u32 syncIndex = (chain->syncIndex + (kResourceSets - 1u)) % kResourceSets;

    vkrImage *const lum = &chain->lumAttachments[imgIndex];
    vkrBuffer *const expBuffer = &ms_expBuffers.frames[syncIndex];
    vkrBuffer *const histBuffer = &ms_histBuffers.frames[syncIndex];

    VkFence cmpfence = NULL;
    VkQueue cmpqueue = NULL;
    VkCommandBuffer cmpcmd = vkrContext_GetTmpCmd(vkrQueueId_Compute, &cmpfence, &cmpqueue);
    vkrCmdBegin(cmpcmd);

    // transition lum img and exposure buf to compute
    {
        VkFence gfxfence = NULL;
        VkQueue gfxqueue = NULL;
        VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(vkrQueueId_Graphics, &gfxfence, &gfxqueue);
        vkrCmdBegin(gfxcmd);

        vkrImage_Transfer(
            lum,
            vkrQueueId_Graphics,
            vkrQueueId_Compute,
            gfxcmd,
            cmpcmd,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkrBuffer_Transfer(
            expBuffer,
            vkrQueueId_Graphics,
            vkrQueueId_Compute,
            gfxcmd,
            cmpcmd,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkrCmdEnd(gfxcmd);
        vkrCmdSubmit(gfxqueue, gfxcmd, gfxfence, NULL, 0x0, NULL);
    }

    // copy exposure buffer to read buffer
    {
        vkrBuffer *const readBuffer = vkrBufferSet_Current(&ms_readbackBuffers);
        vkrBuffer_Barrier(
            readBuffer,
            cmpcmd,
            VK_ACCESS_HOST_READ_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkrCmdCopyBuffer(cmpcmd, expBuffer, readBuffer);
    }

    // dispatch shaders
    {
        const i32 width = chain->width;
        const i32 height = chain->height;
        const PushConstants constants =
        {
            .width = width,
            .height = height,
            .exposure = ms_params,
        };

        // build histogram
        {
            vkrCmdBindPass(cmpcmd, &ms_buildPass);
            vkrCmdPushConstants(cmpcmd, &ms_buildPass, &constants, sizeof(constants));
            const i32 dispatchX = (width + 15) / 16;
            const i32 dispatchY = (height + 15) / 16;
            vkCmdDispatch(cmpcmd, dispatchX, dispatchY, 1);
        }

        vkrBuffer_Barrier(
            histBuffer,
            cmpcmd,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkrBuffer_Barrier(
            expBuffer,
            cmpcmd,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        // adapt exposure
        {
            vkrCmdBindPass(cmpcmd, &ms_adaptPass);
            vkrCmdPushConstants(cmpcmd, &ms_adaptPass, &constants, sizeof(constants));
            vkCmdDispatch(cmpcmd, 1, 1, 1);
        }
    }

    // transition to gfx
    {
        VkFence gfxfence = NULL;
        VkQueue gfxqueue = NULL;
        VkCommandBuffer gfxcmd = vkrContext_GetTmpCmd(vkrQueueId_Graphics, &gfxfence, &gfxqueue);
        vkrCmdBegin(gfxcmd);

        vkrImage_Transfer(
            lum,
            vkrQueueId_Compute,
            vkrQueueId_Graphics,
            cmpcmd,
            gfxcmd,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        vkrBuffer_Transfer(
            expBuffer,
            vkrQueueId_Compute,
            vkrQueueId_Graphics,
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

    ms_lastFence = cmpfence;

    ProfileEnd(pm_execute);
}

vkrExposure* vkrExposure_GetParams(void)
{
    return &ms_params;
}

void vkrExposure_SetParams(const vkrExposure* params)
{
    ms_params = *params;
}

float vkrExposure_GetExposure(void)
{
    return ms_params.exposure;
}

const vkrBuffer* vkrExposure_GetExposureBuffer(void)
{
    return vkrBufferSet_Current(&ms_expBuffers);
}
