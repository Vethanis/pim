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
#include "rendering/vulkan/vkr_exposure.h"
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
    if (!vkrExposure_Init())
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
    vkrExposure_Shutdown();
    vkrUIPass_Del();
}

ProfileMark(pm_setup, vkrMainPass_Setup)
void vkrMainPass_Setup(void)
{
    ProfileBegin(pm_setup);

    vkrDepthPass_Setup();
    vkrOpaquePass_Setup();
    vkrExposure_Setup();
    vkrUIPass_Setup();

    ProfileEnd(pm_setup);
}

ProfileMark(pm_exec, vkrMainPass_Execute)
void vkrMainPass_Execute(void)
{
    ProfileBegin(pm_exec);

    if (ConVar_GetBool(&cv_pt_trace))
    {
        FrameBuf* fbuf = RenderSys_FrontBuf();
        vkrScreenBlit_Blit(
            fbuf->light,
            fbuf->width,
            fbuf->height,
            VK_FORMAT_R32G32B32A32_SFLOAT);
    }
    else
    {
        vkrDepthPass_Execute();
        vkrOpaquePass_Execute();
    }

    vkrExposure_Execute();

    vkrUIPass_Execute();

    ProfileEnd(pm_exec);
}
