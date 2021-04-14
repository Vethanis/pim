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

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/cvars.h"
#include "containers/table.h"
#include "threading/task.h"

#include "math/float4x4_funcs.h"
#include "math/box.h"
#include "math/frustum.h"
#include "math/sdf.h"

#include <string.h>

bool vkrMainPass_New(void)
{
    bool success = true;

    if (!vkrScreenBlit_New())
    {
        success = false;
        goto cleanup;
    }
    if (!vkrDepthPass_New())
    {
        success = false;
        goto cleanup;
    }
    if (!vkrOpaquePass_New())
    {
        success = false;
        goto cleanup;
    }
    if (!vkrUIPass_New())
    {
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to init vkrMainPass");
        vkrMainPass_Del();
    }
    return success;
}

void vkrMainPass_Del(void)
{
    vkrScreenBlit_Del();
    vkrDepthPass_Del();
    vkrOpaquePass_Del();
    vkrUIPass_Del();
}

ProfileMark(pm_setup, vkrMainPass_Setup)
void vkrMainPass_Setup(void)
{
    ProfileBegin(pm_setup);

    vkrDepthPass_Setup();
    vkrOpaquePass_Setup();
    vkrUIPass_Setup();

    ProfileEnd(pm_setup);
}

ProfileMark(pm_exec, vkrMainPass_Execute)
void vkrMainPass_Execute(
    VkCommandBuffer cmd,
    VkFence fence)
{
    ProfileBegin(pm_exec);

    const bool r_sw = ConVar_GetBool(&cv_pt_trace);

    vkrSwapchain *const chain = &g_vkr.chain;
    VkFramebuffer framebuffer = NULL;
    vkrSwapchain_AcquireImage(chain, &framebuffer);
    vkrPassContext passCtx =
    {
        .framebuffer = framebuffer,
        .cmd = cmd,
        .fence = fence,
    };
    if (r_sw)
    {
        FrameBuf* fbuf = RenderSys_FrontBuf();
        vkrScreenBlit_Blit(
            &passCtx,
            fbuf->color,
            fbuf->width,
            fbuf->height,
            VK_FORMAT_R16G16B16A16_UNORM);
    }
    else
    {
        const u32 imageIndex = vkrSys_SwapIndex();
        VkImage colorTarget = chain->images[imageIndex];
        vkrImage* lumTarget = &chain->lumAttachments[imageIndex];
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
            lumTarget,
            cmd,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            0x0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    if (!r_sw)
    {
        vkrDepthPass_Execute(&passCtx);
    }

    if (!r_sw)
    {
        vkrOpaquePass_Execute(&passCtx);
    }

    vkrUIPass_Execute(&passCtx);

    ProfileEnd(pm_exec);
}
