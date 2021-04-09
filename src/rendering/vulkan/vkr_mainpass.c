#include "rendering/vulkan/vkr_mainpass.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/screenblit.h"
#include "rendering/vulkan/vkr_depthpass.h"
#include "rendering/vulkan/vkr_opaquepass.h"
#include "rendering/vulkan/vkr_uipass.h"
#include "rendering/vulkan/vkr_exposurepass.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/drawable.h"
#include "rendering/render_system.h"
#include "rendering/framebuffer.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/lightmap.h"
#include "rendering/camera.h"
#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "math/float4x4_funcs.h"
#include "math/box.h"
#include "math/frustum.h"
#include "math/sdf.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

static VkRenderPass CreateRenderPass(const vkrSwapchain* chain);

static ConVar* cv_pt_trace;

// ----------------------------------------------------------------------------

bool vkrMainPass_New(vkrMainPass *const pass)
{
    ASSERT(pass);

    cv_pt_trace = ConVar_Find("pt_trace");
    ASSERT(cv_pt_trace);

    bool success = true;

    VkRenderPass renderPass = CreateRenderPass(&g_vkr.chain);
    pass->renderPass = renderPass;
    if (!pass->renderPass)
    {
        success = false;
        goto cleanup;
    }

    if (!vkrScreenBlit_New(renderPass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrDepthPass_New(renderPass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrOpaquePass_New(renderPass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrUIPass_New(renderPass))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to init vkrMainPass");
        vkrMainPass_Del(pass);
    }
    return success;
}

void vkrMainPass_Del(vkrMainPass *const pass)
{
    if (pass)
    {
        vkrRenderPass_Del(pass->renderPass);
        vkrScreenBlit_Del();
        vkrDepthPass_Del();
        vkrOpaquePass_Del();
        vkrUIPass_Del();
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_setup, vkrMainPass_Setup)
void vkrMainPass_Setup(vkrMainPass *const pass)
{
    ProfileBegin(pm_setup);

    vkrDepthPass_Setup();
    vkrOpaquePass_Setup();
    vkrUIPass_Setup();

    ProfileEnd(pm_setup);
}

ProfileMark(pm_exec, vkrMainPass_Execute)
void vkrMainPass_Execute(
    vkrMainPass *const pass,
    VkCommandBuffer cmd,
    VkFence fence)
{
    ProfileBegin(pm_exec);

    const bool r_sw = ConVar_GetBool(cv_pt_trace);

    vkrSwapchain *const chain = &g_vkr.chain;
    VkRect2D rect = vkrSwapchain_GetRect(chain);
    const VkClearValue clearValues[] =
    {
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .depthStencil = { 1.0f, 0 },
        },
    };

    VkFramebuffer framebuffer = NULL;
    vkrSwapchain_AcquireImage(chain, &framebuffer);
    vkrPassContext passCtx =
    {
        .renderPass = pass->renderPass,
        .framebuffer = framebuffer,
        .cmd = cmd,
        .fence = fence,
    };
    if (r_sw)
    {
        vkrScreenBlit_Blit(
            &passCtx,
            RenderSys_FrontBuf());
    }
    else
    {
        const u32 imageIndex = vkrSys_SwapIndex();
        VkImage colorTarget = chain->images[imageIndex];
        vkrAttachment* lumTarget = &chain->lumAttachments[imageIndex];
        const VkImageMemoryBarrier colorBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = colorTarget,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            &colorBarrier);
        vkrImage_Barrier(
            &lumTarget->image,
            cmd,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            0x0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    vkrCmdBeginRenderPass(
        cmd,
        pass->renderPass,
        framebuffer,
        rect,
        NELEM(clearValues),
        clearValues,
        VK_SUBPASS_CONTENTS_INLINE);
    passCtx.subpass = 0;

    if (!r_sw)
    {
        vkrDepthPass_Execute(&passCtx);
    }

    vkrCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    passCtx.subpass++;

    if (!r_sw)
    {
        vkrOpaquePass_Execute(&passCtx);
    }

    vkrCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    passCtx.subpass++;

    vkrUIPass_Execute(&passCtx);

    vkrCmdEndRenderPass(cmd);

    ProfileEnd(pm_exec);
}

static VkRenderPass CreateRenderPass(const vkrSwapchain* chain)
{
    const VkAttachmentDescription attachments[] =
    {
        {
            // color
            .format = chain->colorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            // luminance
            .format = chain->lumAttachments[0].image.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            // depth pass depth
            .format = chain->depthAttachments[0].image.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference opaquePassAttachments[] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference uiPassAttachments[] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference depthPassAttachments[] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference depthRef =
    {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpasses[] =
    {
        // depth prepass
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = NELEM(depthPassAttachments),
            .pColorAttachments = depthPassAttachments,
            .pDepthStencilAttachment = &depthRef,
        },
        // opaque pass
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = NELEM(opaquePassAttachments),
            .pColorAttachments = opaquePassAttachments,
            .pDepthStencilAttachment = &depthRef,
        },
        // ui pass
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = NELEM(uiPassAttachments),
            .pColorAttachments = uiPassAttachments,
        },
    };
    // depth load: VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
    // depth store: VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    // color load: VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
    // color store: VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    const VkSubpassDependency dependencies[] =
    {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = vkrPassId_Depth,
            .srcStageMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        },
        {
            .srcSubpass = vkrPassId_Depth,
            .dstSubpass = vkrPassId_Opaque,
            .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        },
        {
            .srcSubpass = vkrPassId_Opaque,
            .dstSubpass = vkrPassId_UI,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        },
    };

    VkRenderPass renderPass = vkrRenderPass_New(
        NELEM(attachments), attachments,
        NELEM(subpasses), subpasses,
        NELEM(dependencies), dependencies);
    ASSERT(renderPass);
    return renderPass;
}
