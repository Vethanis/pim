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

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/scalar.h"
#include <string.h>

static void vkrExposurePass_WriteDesc(const vkrAttachment* img, VkDescriptorSet set);
static void vkrExposurePass_AdaptHistogram(vkrExposurePass* pass, u32* pim_noalias histogram);

bool vkrExposurePass_New(vkrExposurePass* pass)
{
    ASSERT(pass);
    memset(pass, 0, sizeof(*pass));
    bool success = true;

    VkPipelineShaderStageCreateInfo shader = { 0 };
    if (!vkrShader_New(&shader, "BuildHistogram.hlsl", "CSMain", vkrShaderType_Comp))
    {
        success = false;
        goto cleanup;
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
    };
    VkDescriptorSetLayout setLayout = vkrSetLayout_New(NELEM(bindings), bindings, 0x0);
    pass->setLayout = setLayout;
    if (!setLayout)
    {
        success = false;
        goto cleanup;
    }
    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .size = sizeof(vkrExposureConstants),
        },
    };
    VkPipelineLayout layout = vkrPipelineLayout_New(1, &setLayout, NELEM(ranges), ranges);
    pass->layout = layout;
    if (!layout)
    {
        success = false;
        goto cleanup;
    }
    VkPipeline pipeline = vkrPipeline_NewComp(layout, &shader);
    pass->pipeline = pipeline;
    if (!pipeline)
    {
        success = false;
        goto cleanup;
    }
    const VkDescriptorPoolSize descSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
        },
    };
    VkDescriptorPool descPool = vkrDescPool_New(kFramesInFlight, NELEM(descSizes), descSizes);
    pass->descPool = descPool;
    if (!descPool)
    {
        success = false;
        goto cleanup;
    }

    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        VkDescriptorSet set = vkrDesc_New(descPool, setLayout);
        pass->sets[i] = set;
        if (!set)
        {
            success = false;
            goto cleanup;
        }
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

    {
        vkrBinding histBindings[kFramesInFlight] = { 0 };
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            histBindings[i].set = pass->sets[i];
            histBindings[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            histBindings[i].binding = 1;
            histBindings[i].buffer.buffer = pass->histBuffers[i].handle;
            histBindings[i].buffer.range = pass->histBuffers[i].size;
        }
        vkrDesc_WriteBindings(NELEM(histBindings), histBindings);
    }

cleanup:
    vkrShader_Del(&shader);
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
        vkrPipeline_Del(pass->pipeline);
        vkrDescPool_Del(pass->descPool);
        vkrPipelineLayout_Del(pass->layout);
        vkrSetLayout_Del(pass->setLayout);
        for (i32 i = 0; i < NELEM(pass->histBuffers); ++i)
        {
            vkrBuffer_Del(&pass->histBuffers[i]);
        }
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_execute, vkrExposurePass_Execute)
void vkrExposurePass_Execute(vkrExposurePass* pass)
{
    ProfileBegin(pm_execute);

    const vkrSwapchain* chain = &g_vkr.chain;
    u32 imgIndex = chain->imageIndex;
    u32 syncIndex = chain->syncIndex;
    const vkrAttachment* lum = &chain->lumAttachments[imgIndex];

    u32 gfxFamily = g_vkr.queues[vkrQueueId_Gfx].family;
    u32 cmpFamily = g_vkr.queues[vkrQueueId_Comp].family;

    vkrThreadContext* ctx = vkrContext_Get();

    // readback oldest results to minimize sync point
    {
        vkrBuffer* buffer = &pass->histBuffers[syncIndex];
        {
            // transition to host r
            VkFence fence = NULL;
            VkQueue queue = NULL;
            VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &fence, &queue);
            vkrCmdBegin(cmd);
            const VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buffer->handle,
                .offset = 0,
                .size = buffer->size,
            };
            vkrCmdBufferBarrier(
                cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                &barrier);
            vkrCmdEnd(cmd);
            vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
            // blocking wait to ensure writes become cpu visible
            vkrFence_Wait(fence);
        }
        {
            u32* histogram = vkrBuffer_Map(buffer);
            vkrExposurePass_AdaptHistogram(pass, histogram);
            vkrBuffer_Unmap(buffer);
            vkrBuffer_Flush(buffer);
        }
        {
            // transition back to shader rw
            VkFence fence = NULL;
            VkQueue queue = NULL;
            VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &fence, &queue);
            vkrCmdBegin(cmd);
            const VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buffer->handle,
                .offset = 0,
                .size = buffer->size,
            };
            vkrCmdBufferBarrier(
                cmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                &barrier);
            vkrCmdEnd(cmd);
            vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
        }
    }

    // queue ownership transfer from gfx to compute
    // transition from attachment to shader rw
    const VkImageMemoryBarrier toCompute =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = gfxFamily,
        .dstQueueFamilyIndex = cmpFamily,
        .image = lum->image.handle,
        .subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    // queue ownership transfer from compute to gfx
    // transition from shader rw to attachment
    const VkImageMemoryBarrier toGraphics =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = cmpFamily,
        .dstQueueFamilyIndex = gfxFamily,
        .image = lum->image.handle,
        .subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    // transition to compute (give)
    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &fence, &queue);
        vkrCmdBegin(cmd);
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            &toCompute);
        vkrCmdEnd(cmd);
        vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    }

    // transition to compute (take)
    // dispatch histogram shader
    // transition to gfx (give)
    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Comp, &fence, &queue);
        vkrCmdBegin(cmd);
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            &toCompute);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->pipeline);
        VkDescriptorSet set = pass->sets[syncIndex];
        vkrExposurePass_WriteDesc(lum, set);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->layout, 0, 1, &set, 0, NULL);
        i32 width = chain->width;
        i32 height = chain->height;
        const vkrExposureConstants constants =
        {
            .width = width,
            .height = height,
            .minEV = pass->params.minEV,
            .maxEV = pass->params.maxEV,
        };
        vkCmdPushConstants(
            cmd, pass->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(vkrExposureConstants), &constants);
        i32 dispatchX = (width + 15) / 16;
        i32 dispatchY = (height + 15) / 16;
        vkCmdDispatch(cmd, dispatchX, dispatchY, 1);
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            &toGraphics);
        vkrCmdEnd(cmd);
        vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    }

    // transition to gfx (take)
    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, vkrQueueId_Gfx, &fence, &queue);
        vkrCmdBegin(cmd);
        vkrCmdImageBarrier(
            cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            &toGraphics);
        vkrCmdEnd(cmd);
        vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    }

    ProfileEnd(pm_execute);
}

static void vkrExposurePass_WriteDesc(const vkrAttachment* img, VkDescriptorSet set)
{
    const vkrBinding imgBindings[] =
    {
        {
            .set = set,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .binding = 0,
            .image =
            {
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .imageView = img->view,
            },
        },
    };
    vkrDesc_WriteBindings(NELEM(imgBindings), imgBindings);
}

static float LumToEV100(float Lavg)
{
    // EV100 = log2((Lavg * S) / K)
    // S = 100
    // K = 12.5
    // S/K = 8
    return log2f(Lavg * 8.0f);
}

// https://www.desmos.com/calculator/vorr8hwdl7
static float EV100ToLum(float ev100)
{
    return exp2f(ev100) / 8.0f;
}

static float BinToEV(i32 i, float minEV, float dEV)
{
    // i varies from 1 to 255
    // reciprocal range is: 1.0 / 254.0
    float ev = minEV + (i - 1) * dEV;
    // log2(kEpsilon) == -22
    ev = i > 0 ? ev : -22.0f;
    return ev;
}

static float AdaptLuminance(float lum0, float lum1, float dt, float tau)
{
    float t = 1.0f - expf(-dt * tau);
    return f1_lerp(lum0, lum1, f1_sat(t));
}

static float ManualEV100(float aperture, float shutterTime, float ISO)
{
    float a = (aperture * aperture) / shutterTime;
    float b = 100.0f / ISO;
    return log2f(a * b);
}

static float SaturationExposure(float ev100)
{
    const float factor = 78.0f / (100.0f * 0.65f);
    float Lmax = factor * EV100ToLum(ev100);
    return 1.0f / Lmax;
}

static float StandardExposure(float ev100)
{
    const float midGrey = 0.18f;
    const float factor = 10.0f / (100.0f * 0.65f);
    float Lavg = factor * EV100ToLum(ev100);
    return midGrey / Lavg;
}

static float CalcExposure(const vkrExposure* args)
{
    float ev100;
    if (args->manual)
    {
        ev100 = ManualEV100(args->aperture, args->shutterTime, args->ISO);
    }
    else
    {
        ev100 = LumToEV100(args->avgLum);
    }

    ev100 = ev100 - args->offsetEV;

    float exposure;
    if (args->standard)
    {
        exposure = StandardExposure(ev100);
    }
    else
    {
        exposure = SaturationExposure(ev100);
    }
    return exposure;
}

static void vkrExposurePass_AdaptHistogram(vkrExposurePass* pass, u32* pim_noalias histogram)
{
    ASSERT(histogram);
    i32 width = g_vkr.chain.width;
    i32 height = g_vkr.chain.height;
    const float rcpSamples = 1.0f / (width * height);
    const float minEV = pass->params.minEV;
    const float maxEV = pass->params.maxEV;
    const float minCdf = pass->params.minCdf;
    const float maxCdf = pass->params.maxCdf;
    const float dEV = (maxEV - minEV) * (1.0f / (kHistogramSize - 2));

    // histogram seems to be in an odd memory region that is slow to access
    // copy it to stack before going further
    u32 stackgram[kHistogramSize];
    memcpy(stackgram, histogram, sizeof(stackgram));
    memset(histogram, 0, sizeof(histogram[0]) * kHistogramSize);

    u32 samples = 0;
    for (i32 i = 0; i < kHistogramSize; ++i)
    {
        samples += stackgram[i];
    }

    float avgLum = 0.0f;
    float cdf = 0.0f;
    for (i32 i = 0; i < kHistogramSize; ++i)
    {
        u32 count = stackgram[i];
        float pdf = rcpSamples * count;

        // https://www.desmos.com/calculator/tetw9t35df
        float rcpPdf = 1.0f / f1_max(kEpsilon, pdf);
        float w0 = 1.0f - f1_sat((minCdf - cdf) * rcpPdf);
        float w1 = f1_sat((maxCdf - cdf) * rcpPdf);
        float w = pdf * w0 * w1;

        float ev = BinToEV(i, minEV, dEV);
        float lum = EV100ToLum(ev);

        avgLum += lum * pdf;
        cdf += pdf;
    }

    pass->params.deltaTime = (float)time_dtf();
    pass->params.avgLum = AdaptLuminance(
        pass->params.avgLum,
        avgLum,
        pass->params.deltaTime,
        pass->params.adaptRate);
    pass->params.exposure = CalcExposure(&pass->params);
}
