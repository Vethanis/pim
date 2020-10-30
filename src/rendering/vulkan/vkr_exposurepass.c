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

#include "rendering/exposure.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

bool vkrExposurePass_New(vkrExposurePass* pass)
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
    const VkDescriptorSetLayoutBinding bindings[] =
    {
        {
            // input luminance storage image. must be in layout VK_IMAGE_LAYOUT_GENERAL
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        {
            // histogram storage buffer
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        {
            // exposure storage buffer
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .size = sizeof(vkrExposureConstants),
        },
    };
    const VkDescriptorPoolSize poolSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
        },
    };

    const vkrPassDesc desc =
    {
        .bindpoint = VK_PIPELINE_BIND_POINT_COMPUTE,
        .poolSizeCount = NELEM(poolSizes),
        .poolSizes = poolSizes,
        .bindingCount = NELEM(bindings),
        .bindings = bindings,
        .rangeCount = NELEM(ranges),
        .ranges = ranges,
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

void vkrExposurePass_Del(vkrExposurePass* pass)
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

ProfileMark(pm_execute, vkrExposurePass_Execute)
void vkrExposurePass_Execute(vkrExposurePass* pass)
{
    ProfileBegin(pm_execute);

    pass->params.deltaTime = f1_lerp(pass->params.deltaTime, (float)time_dtf(), 0.25f);

    vkrSwapchain* chain = &g_vkr.chain;
    const u32 chainLen = chain->length;
    const u32 imgIndex = (chain->imageIndex + (chainLen - 1u)) % chainLen;
    const u32 syncIndex = (chain->syncIndex + (kFramesInFlight - 1u)) % kFramesInFlight;

    vkrAttachment* lum = &chain->lumAttachments[imgIndex];
    vkrBuffer* expBuffer = &pass->expBuffers[syncIndex];
    vkrBuffer* histBuffer = &pass->histBuffers[syncIndex];

    VkDescriptorSet set = pass->pass.sets[syncIndex];
    VkPipelineLayout layout = pass->pass.layout;
    VkPipeline buildPipe = pass->pass.pipeline;
    VkPipeline adaptPipe = pass->adapt;

    const u32 gfxFamily = g_vkr.queues[vkrQueueId_Gfx].family;
    const u32 cmpFamily = g_vkr.queues[vkrQueueId_Comp].family;

    vkrThreadContext* ctx = vkrContext_Get();

    // update lum binding
    {
        const vkrBinding bindings[] =
        {
            {
                .set = set,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .binding = 0,
                .image =
                {
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .imageView = lum->view,
                },
            },
            {
                .set = set,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .binding = 1,
                .buffer =
                {
                    .buffer = histBuffer->handle,
                    .range = VK_WHOLE_SIZE,
                },
            },
            {
                .set = set,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .binding = 3,
                .buffer =
                {
                    .buffer = expBuffer->handle,
                    .range = VK_WHOLE_SIZE,
                },
            },
        };
        vkrDesc_WriteBindings(NELEM(bindings), bindings);
    }

    // transition lum img and exposure buf to compute
    {
        vkrImage_Transfer(
            &lum->image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            vkrQueueId_Gfx,
            vkrQueueId_Comp);
        vkrBuffer_Transfer(
            expBuffer,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            vkrQueueId_Gfx,
            vkrQueueId_Comp);
    }

    // dispatch shaders
    {
        VkFence cmpfence = NULL;
        VkQueue cmpqueue = NULL;
        VkCommandBuffer cmpcmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &cmpfence, &cmpqueue);
        vkrCmdBegin(cmpcmd);

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

        vkrCmdEnd(cmpcmd);
        vkrCmdSubmit(cmpqueue, cmpcmd, cmpfence, NULL, 0x0, NULL);
    }

    // transition to gfx
    {
        vkrImage_Transfer(
            &lum->image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            vkrQueueId_Comp,
            vkrQueueId_Gfx);
    }

    ProfileEnd(pm_execute);
}
