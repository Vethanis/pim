#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_debug.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_framebuffer.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mainpass.h"
#include "rendering/vulkan/vkr_exposure.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/vulkan/vkr_im.h"
#include "rendering/vulkan/vkr_sampler.h"

#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/render_system.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/screenblit.h"
#include "rendering/lightmap.h"
#include "rendering/r_constants.h"

#include "ui/cimgui_ext.h"
#include "ui/ui.h"

#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/cvars.h"
#include "common/atomics.h"
#include "containers/table.h"
#include "threading/task.h"

#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/quat_funcs.h"

#include <string.h>

static void vkrUploadLightmaps(void);

vkrSys g_vkr;
vkrLayers g_vkrLayers;
vkrInstExts g_vkrInstExts;
vkrDevExts g_vkrDevExts;
vkrProps g_vkrProps;
vkrFeats g_vkrFeats;

bool vkrSys_Init(void)
{
    memset(&g_vkr, 0, sizeof(g_vkr));

    bool success = true;

    if (!vkrInstance_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    i32 width = 0;
    i32 height = 0;
    if (!vkrDisplay_MonitorSize(&width, &height))
    {
        success = false;
        goto cleanup;
    }
    r_width_set(width);
    r_height_set(height);
    if (!vkrDisplay_New(&g_vkr.display, width, height, "pim"))
    {
        success = false;
        goto cleanup;
    }
    UiSys_Init(g_vkr.display.window);

    if (!vkrDevice_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMemSys_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrSwapchain_New(&g_vkr.chain, &g_vkr.display, NULL))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrContext_New(&g_vkr.context))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrSampler_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrTexTable_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrBindings_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMeshSys_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrImSys_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMainPass_New())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrExposure_Init())
    {
        success = false;
        goto cleanup;
    }

cleanup:

    if (!success)
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to init vkrSys");
        vkrSys_Shutdown();
    }
    return success;
}

ProfileMark(pm_update, vkrSys_Update)
void vkrSys_Update(void)
{
    if (!g_vkr.inst)
    {
        return;
    }
    vkrDisplay* display = &g_vkr.display;
    vkrSwapchain* chain = &g_vkr.chain;

    // swapchain re-creation
    {
        if (!vkrDisplay_IsOpen(display))
        {
            return;
        }
        if (vkrDisplay_UpdateSize(display))
        {
            if (vkrSwapchain_Recreate(
                chain,
                display))
            {
                r_width_set(display->width);
                r_height_set(display->height);
            }
        }
        if (!chain->handle)
        {
            return;
        }
    }

    ProfileBegin(pm_update);

    VkFence fence = NULL;
    VkCommandBuffer cmd = NULL;

    // base system update
    {
        vkrSwapchain_AcquireSync(chain, &cmd, &fence);
        vkrMemSys_Update();
    }

    // system update
    {
        vkrSampler_Update();
        vkrMeshSys_Update();
        vkrImSys_Flush(NULL);
    }

    // setup phase
    {
        vkrExposure_Setup();
        vkrMainPass_Setup();
        vkrTexTable_Update();
        vkrBindings_Update();
    }

    // execute phase
    {
        vkrExposure_Execute();
        vkrMainPass_Execute(cmd, fence);
    }

    // present phase
    {
        vkrSwapchain_Submit(chain, cmd);
        vkrSwapchain_Present(chain);
    }

    // background work
    {
        vkrUploadLightmaps();
    }

    // setup for next frame
    {
        vkrImSys_Clear();
    }

    ProfileEnd(pm_update);
}

void vkrSys_Shutdown(void)
{
    if (g_vkr.inst)
    {
        vkrDevice_WaitIdle();

        LmPack_Del(LmPack_Get());
        UiSys_Shutdown();

        vkrExposure_Shutdown();
        vkrMainPass_Del();

        vkrImSys_Shutdown();
        vkrMeshSys_Shutdown();
        vkrBindings_Shutdown();
        vkrTexTable_Shutdown();
        vkrSampler_Shutdown();
        vkrRenderPass_Clear();

        vkrMemSys_Finalize();

        vkrContext_Del(&g_vkr.context);
        vkrSwapchain_Del(&g_vkr.chain);
        vkrMemSys_Shutdown();
        vkrDevice_Shutdown(&g_vkr);
        vkrDisplay_Del(&g_vkr.display);
        vkrInstance_Shutdown(&g_vkr);
    }
}

void vkrSys_OnLoad(void)
{
    vkrMemSys_Update();
}

void vkrSys_OnUnload(void)
{
    vkrMemSys_Update();
}

u32 vkrSys_SyncIndex(void) { return g_vkr.chain.syncIndex; }
u32 vkrSys_SwapIndex(void) { return g_vkr.chain.imageIndex; }
u32 vkrSys_FrameIndex(void) { return Time_FrameCount(); }

bool vkrSys_HdrEnabled(void)
{
    switch (g_vkr.chain.colorSpace)
    {
    default:
        return false;
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: // Rec2100 w/ PQ OETF
        return true;
    case VK_COLOR_SPACE_HDR10_HLG_EXT: // Rec2100 w/ HLG OETF
        return true;
    }
}

float vkrSys_GetWhitepoint(void)
{
    return ConVar_GetFloat(&cv_r_whitepoint);
}

float vkrSys_GetDisplayNitsMin(void)
{
    return ConVar_GetFloat(&cv_r_display_nits_min);
}

float vkrSys_GetDisplayNitsMax(void)
{
    return ConVar_GetFloat(&cv_r_display_nits_max);
}

float vkrSys_GetUiNits(void)
{
    return ConVar_GetFloat(&cv_r_ui_nits);
}

Colorspace vkrSys_GetRenderColorspace(void)
{
#if COLOR_SCENE_REC709
    return Colorspace_Rec709;
#elif COLOR_SCENE_REC2020
    return Colorspace_Rec2020;
#elif COLOR_SCENE_AP1
    return Colorspace_AP1;
#elif COLOR_SCENE_AP0
    return Colorspace_AP0;
#else
#   error Unrecognized scene colorspace
#endif // COLOR_SCENE_X
}

Colorspace vkrSys_GetDisplayColorspace(void)
{
    switch (g_vkr.chain.colorSpace)
    {
    default:
        return Colorspace_Rec709;
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: // Rec2100 w/ PQ OETF
        return Colorspace_Rec2020;
    case VK_COLOR_SPACE_HDR10_HLG_EXT: // Rec2100 w/ HLG OETF
        return Colorspace_Rec2020;
    }
}

ProfileMark(pm_uplm, vkrUploadLightmaps)
static void vkrUploadLightmaps(void)
{
    ProfileBegin(pm_uplm);
    if (ConVar_GetBool(&cv_lm_upload))
    {
        ConVar_SetBool(&cv_lm_upload, false);

        LmPack* pack = LmPack_Get();
        for (i32 i = 0; i < pack->lmCount; ++i)
        {
            Lightmap* lm = pack->lightmaps + i;
            Lightmap_Upload(lm);
        }
    }
    ProfileEnd(pm_uplm);
}
