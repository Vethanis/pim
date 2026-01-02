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
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_framebuffer.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_sampler.h"

#include "rendering/exposure.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/cvars.h"
#include "math/scalar.h"
#include "math/color.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct PushConstants_s
{
    u32 width;
    u32 height;
    vkrExposure exposure;
} PushConstants;

typedef struct ExposureBuffer_s
{
    pim_alignas(16)
    float averageLum;
    float exposure;
    float maxLum;
    float minLum;
} ExposureBuffer;

// ----------------------------------------------------------------------------

static void vkrExposure_Readback(void);

// ----------------------------------------------------------------------------

static vkrPass ms_clearPass;
static vkrPass ms_buildPass;
static vkrPass ms_adaptPass;
static vkrPass ms_exposePass;
static VkRenderPass ms_exposeRenderPass;
static vkrBufferSet ms_histBuffer;
static vkrBufferSet ms_expBuffer;
static vkrBufferSet ms_readbackBuffer;
static vkrSubmitId ms_readbacks[R_ResourceSets];
static float4 ms_averageValue; // ExposureBuffer
static vkrExposure ms_params;

// ----------------------------------------------------------------------------

vkrExposure* vkrExposure_GetParams(void) { return &ms_params; }
float vkrGetAvgLuminance(void) { return ms_averageValue.x; }
float vkrGetExposure(void) { return ms_averageValue.y; }
float vkrGetMaxLuminance(void) { return ms_averageValue.z; }
float vkrGetMinLuminance(void) { return ms_averageValue.w; }

bool vkrExposure_Init(void)
{
    bool success = true;

    VkPipelineShaderStageCreateInfo clearShaders[1] = { 0 };
    VkPipelineShaderStageCreateInfo buildShaders[1] = { 0 };
    VkPipelineShaderStageCreateInfo adaptShaders[1] = { 0 };
    VkPipelineShaderStageCreateInfo exposeShaders[2] = { 0 };

    {
        if (!vkrShader_New(
            &clearShaders[0],
            "ClearHistogram.hlsl", "CSMain", vkrShaderType_Comp))
        {
            success = false;
            goto cleanup;
        }
        const vkrPassDesc desc =
        {
            .pushConstantBytes = 0,
            .shaderCount = NELEM(clearShaders),
            .shaders = clearShaders,
        };
        if (!vkrPass_New(&ms_clearPass, &desc))
        {
            success = false;
            goto cleanup;
        }
    }

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

    {
        const vkrImage* backBuffer = vkrGetBackBuffer();
        const vkrRenderPassDesc renderPassDesc =
        {
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .attachments[0] =
            {
                .format = backBuffer->format,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .load = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .store = VK_ATTACHMENT_STORE_OP_STORE,
            },
        };
        ms_exposeRenderPass = vkrRenderPass_Get(&renderPassDesc);
        if (!ms_exposeRenderPass)
        {
            success = false;
            goto cleanup;
        }

        if (!vkrShader_New(
            &exposeShaders[0],
            "post.hlsl", "VSMain", vkrShaderType_Vert))
        {
            success = false;
            goto cleanup;
        }
        if (!vkrShader_New(
            &exposeShaders[1],
            "post.hlsl", "PSMain", vkrShaderType_Frag))
        {
            success = false;
            goto cleanup;
        }

        const vkrPassDesc passDesc =
        {
            .pushConstantBytes = 0,
            .renderPass = ms_exposeRenderPass,
            .subpass = 0,
            .vertLayout =
            {
                .bindingCount = 0,
                .bindings = NULL,
                .attributeCount = 0,
                .attributes = NULL,
            },
            .fixedFuncs =
            {
                .viewport = vkrSwapchain_GetViewport(),
                .scissor = vkrSwapchain_GetRect(),
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .cullMode = VK_CULL_MODE_NONE,
                .depthCompareOp = VK_COMPARE_OP_ALWAYS,
                .scissorOn = false,
                .depthClamp = false,
                .depthTestEnable = false,
                .depthWriteEnable = false,
                .attachmentCount = 1,
                .attachments[0] =
                {
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                    .blendEnable = false,
                },
            },
            .shaderCount = NELEM(exposeShaders),
            .shaders = exposeShaders,
        };
        if (!vkrPass_New(&ms_exposePass, &passDesc))
        {
            success = false;
            goto cleanup;
        }
    }


cleanup:
    for (i32 i = 0; i < NELEM(clearShaders); ++i)
    {
        vkrShader_Del(&clearShaders[i]);
    }
    for (i32 i = 0; i < NELEM(buildShaders); ++i)
    {
        vkrShader_Del(&buildShaders[i]);
    }
    for (i32 i = 0; i < NELEM(adaptShaders); ++i)
    {
        vkrShader_Del(&adaptShaders[i]);
    }
    for (i32 i = 0; i < NELEM(exposeShaders); ++i)
    {
        vkrShader_Del(&exposeShaders[i]);
    }
    if (!success)
    {
        vkrExposure_Shutdown();
    }
    return success;
}

void vkrExposure_Shutdown(void)
{
    vkrPass_Del(&ms_clearPass);
    vkrPass_Del(&ms_buildPass);
    vkrPass_Del(&ms_adaptPass);
    vkrPass_Del(&ms_exposePass);
    vkrBufferSet_Release(&ms_histBuffer);
    vkrBufferSet_Release(&ms_expBuffer);
    vkrBufferSet_Release(&ms_readbackBuffer);
}

ProfileMark(pm_setup, vkrExposure_Setup)
void vkrExposure_Setup(void)
{
    ProfileBegin(pm_setup);

    {
        ms_params.manual = ConVar_GetBool(&cv_exp_manual);
        ms_params.standard = ConVar_GetBool(&cv_exp_standard);
        ms_params.offsetEV = ConVar_GetFloat(&cv_exp_evoffset);
        ms_params.aperture = ConVar_GetFloat(&cv_exp_aperture);
        ms_params.shutterTime = ConVar_GetFloat(&cv_exp_shutter);
        ms_params.adaptRate = ConVar_GetFloat(&cv_exp_adaptrate);
        ms_params.minEV = ConVar_GetFloat(&cv_exp_evmin);
        ms_params.maxEV = ConVar_GetFloat(&cv_exp_evmax);
        ms_params.minCdf = ConVar_GetFloat(&cv_exp_cdfmin);
        ms_params.maxCdf = ConVar_GetFloat(&cv_exp_cdfmax);
        ms_params.ISO = 100.0f;
    }

    vkrExposure_Readback();

    vkrBuffer* expBuffer = vkrBufferSet_Current(&ms_expBuffer);
    vkrBuffer* histBuffer = vkrBufferSet_Current(&ms_histBuffer);

    vkrBuffer_Reserve(
        expBuffer,
        sizeof(float4),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_GpuOnly);
    vkrBuffer_Reserve(
        histBuffer,
        sizeof(u32) * R_ExposureHistogramSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        vkrMemUsage_GpuOnly);

    // copy prior exposure buffer to prior read buffer
    {
        vkrBuffer* nextExpBuffer = vkrBufferSet_Next(&ms_expBuffer);
        if (nextExpBuffer->handle)
        {
            vkrBuffer* nextReadBuffer = vkrBufferSet_Next(&ms_readbackBuffer);
            vkrBuffer_Reserve(
                nextReadBuffer,
                sizeof(float4),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                vkrMemUsage_CpuOnly);
            vkrCmdBuf* cmd_x = vkrCmdGet_G();
            vkrBufferState_TransferSrc(cmd_x, nextExpBuffer);
            vkrBufferState_TransferDst(cmd_x, nextReadBuffer);
            vkrCmdCopyBuffer(cmd_x, nextExpBuffer, nextReadBuffer);
            vkrBufferState_HostRead(cmd_x, nextReadBuffer);
            ms_readbacks[vkrGetNextSyncIndex()] = vkrCmdSubmit(cmd_x, NULL, 0x0, NULL);
        }
    }

    vkrImage* lum = vkrGetSceneBuffer();
    VkSampler lumSampler = vkrSampler_Get(
        VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        (float)R_AnisotropySamples);

    vkrBindings_BindImage(
        bid_SceneLuminance,
        bid_SceneLuminance_TYPE,
        lumSampler,
        lum->view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkrBindings_BindBuffer(
        bid_HistogramBuffer,
        bid_HistogramBuffer_TYPE,
        histBuffer);
    vkrBindings_BindBuffer(
        bid_ExposureBuffer,
        bid_ExposureBuffer_TYPE,
        expBuffer);

    ProfileEnd(pm_setup);
}

ProfileMark(pm_execute, vkrExposure_Execute)
void vkrExposure_Execute(void)
{
    ProfileBegin(pm_execute);

    vkrBuffer* expBuffer = vkrBufferSet_Current(&ms_expBuffer);
    vkrImage* sceneLum = vkrGetSceneBuffer();

    // dispatch shaders
    {
        vkrCmdBuf* cmd_c = vkrCmdGet_G();
        vkrBuffer* histBuffer = vkrBufferSet_Current(&ms_histBuffer);

        const i32 width = sceneLum->width;
        const i32 height = sceneLum->height;
        const PushConstants constants =
        {
            .width = width,
            .height = height,
            .exposure = ms_params,
        };

        // clear histogram
        {
            vkrBufferState_ComputeStore(cmd_c, histBuffer);
            vkrCmdBindPass(cmd_c, &ms_clearPass);
            const i32 dispatchX = (R_ExposureHistogramSize + 31) / 32;
            vkrCmdDispatch(cmd_c, dispatchX, 1, 1);
        }

        // build histogram
        {
            vkrImageState_ComputeLoad(cmd_c, sceneLum);
            vkrBufferState_ComputeLoadStore(cmd_c, histBuffer);
            vkrCmdBindPass(cmd_c, &ms_buildPass);
            vkrCmdPushConstants(cmd_c, &ms_buildPass, &constants, sizeof(constants));
            const i32 dispatchX = (width + 15) / 16;
            const i32 dispatchY = (height + 15) / 16;
            vkrCmdDispatch(cmd_c, dispatchX, dispatchY, 1);
        }

        // adapt exposure
        {
            vkrBufferState_ComputeLoad(cmd_c, histBuffer);
            vkrBufferState_ComputeStore(cmd_c, expBuffer);
            vkrCmdBindPass(cmd_c, &ms_adaptPass);
            vkrCmdPushConstants(cmd_c, &ms_adaptPass, &constants, sizeof(constants));
            vkrCmdDispatch(cmd_c, 1, 1, 1);
        }

    }

    {
        vkrCmdBuf* cmd_g = vkrCmdGet_G();
        vkrImage* backbuffer = vkrGetBackBuffer();

        vkrBufferState_FragLoad(cmd_g, expBuffer);
        vkrImageState_FragSample(cmd_g, sceneLum);
        vkrImageState_ColorAttachWrite(cmd_g, backbuffer);

        const vkrImage* attachments[] =
        {
            backbuffer,
        };
        VkRect2D rect = { .extent = { attachments[0]->width, attachments[0]->height } };
        VkFramebuffer framebuffer = vkrFramebuffer_Get(
            attachments, NELEM(attachments),
            rect.extent.width, rect.extent.height);

        vkrCmdDefaultViewport(cmd_g);
        vkrCmdBindPass(cmd_g, &ms_exposePass);
        const VkClearValue clearValues[] =
        {
            {
                .color.float32 = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        };
        vkrCmdBeginRenderPass(
            cmd_g,
            ms_exposeRenderPass,
            framebuffer,
            rect,
            NELEM(clearValues), clearValues);
        vkrCmdDraw(cmd_g, 3, 0);
        vkrCmdEndRenderPass(cmd_g);
    }

    ProfileEnd(pm_execute);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_readback, vkrExposure_Readback)
static void vkrExposure_Readback(void)
{
    ProfileBegin(pm_readback);

    ms_params.deltaTime = f1_sat((float)Time_SmoothDeltaf());

    // readback prior frames exposure info
    vkrSubmitId readSubmit = ms_readbacks[vkrGetSyncIndex()];
    if (readSubmit.valid)
    {
        vkrSubmit_Await(readSubmit);
        float4 readbackValue = f4_0;
        vkrBuffer* readBuffer = vkrBufferSet_Current(&ms_readbackBuffer);
        if (vkrBuffer_Read(readBuffer, &readbackValue, sizeof(readbackValue)))
        {
            ms_params.exposure = readbackValue.y;
            ms_averageValue = f4_lerpvs(ms_averageValue, readbackValue, ms_params.deltaTime);
        }
    }
    ms_params.avgLum = ms_averageValue.x;

    // set monitor metadata
    if (g_vkrDevExts.EXT_hdr_metadata && vkrGetHdrEnabled())
    {
        const float minMonitorNits = vkrGetDisplayNitsMin();
        const float maxMonitorNits = vkrGetDisplayNitsMax();
        const float4x2 primaries = Colorspace_GetPrimaries(vkrGetDisplayColorspace());
        // convert scene luminance to display luminance
        const float4 averaged = f4_PQ_OOTF(ms_averageValue);
        const float avgContentLum = averaged.x;
        const float maxContentLum = averaged.z;
        const VkHdrMetadataEXT metadata =
        {
            .sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT,
            .displayPrimaryRed.x = primaries.c0.x,
            .displayPrimaryRed.y = primaries.c1.x,
            .displayPrimaryGreen.x = primaries.c0.y,
            .displayPrimaryGreen.y = primaries.c1.y,
            .displayPrimaryBlue.x = primaries.c0.z,
            .displayPrimaryBlue.y = primaries.c1.z,
            .whitePoint.x = primaries.c0.w,
            .whitePoint.y = primaries.c1.w,
            .minLuminance = minMonitorNits, // minimum luminance of the reference monitor in nits
            .maxLuminance = maxMonitorNits, // maximum luminance of the reference monitor in nits
            .maxFrameAverageLightLevel = avgContentLum, // MaxFALL (content's maximum frame average light level in nits)
            .maxContentLightLevel = maxContentLum, // MaxCLL (content�s maximum luminance in nits)
        };
        vkSetHdrMetadataEXT(g_vkr.dev, 1, &g_vkr.chain.handle, &metadata);
    }

    ProfileEnd(pm_readback);
}
