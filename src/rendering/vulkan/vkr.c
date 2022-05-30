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
#include "rendering/vulkan/vkr_uipass.h"
#include "rendering/vulkan/vkr_targets.h"

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

bool vkrSys_WindowInit(void)
{
    const bool fullscreen = ConVar_GetBool(&cv_fullscreen);
    i32 width = 0;
    i32 height = 0;
    if (!vkrDisplay_GetSize(&width, &height, fullscreen))
    {
        return false;
    }
    if (!vkrWindow_New(&g_vkr.window, "pim", width, height, fullscreen))
    {
        return false;
    }
    r_width_set(width);
    r_height_set(height);
    UiSys_Init(g_vkr.window.handle);
    return true;
}

void vkrSys_WindowShutdown(void)
{
    UiSys_Shutdown();
    r_width_set(0);
    r_height_set(0);
    vkrWindow_Del(&g_vkr.window);
}

bool vkrSys_WindowUpdate(void)
{
    vkrWindow* window = &g_vkr.window;
    vkrSwapchain* chain = &g_vkr.chain;

    if (!g_vkr.inst)
    {
        return false;
    }
    if (!vkrWindow_IsOpen(window))
    {
        return false;
    }
    if (ConVar_GetBool(&cv_fullscreen) != window->fullscreen)
    {
        vkrDevice_WaitIdle();

        vkrUIPass_Del();
        vkrSwapchain_Del(chain);
        vkrSys_WindowShutdown();

        vkrSys_WindowInit();
        vkrSwapchain_New(chain, NULL);
        vkrUIPass_New();
    }
    if (vkrWindow_UpdateSize(window))
    {
        if (vkrSwapchain_Recreate())
        {
            r_width_set(window->width);
            r_height_set(window->height);
        }
    }
    if (!chain->handle)
    {
        return false;
    }
    vkrTargets_Recreate();
    return true;
}

bool vkrSys_Init(void)
{
    memset(&g_vkr, 0, sizeof(g_vkr));

    bool success = true;

    if (!vkrInstance_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrSys_WindowInit())
    {
        success = false;
        goto cleanup;
    }

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

    if (!vkrFramebuffer_Init())
    {
        success = false;
        goto cleanup;
    }

    if (!vkrSwapchain_New(&g_vkr.chain, NULL))
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

    if (!vkrTargets_Init())
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
    ProfileBegin(pm_update);

    // base system update
    {
        vkrSwapchain_AcquireSync();
        vkrSwapchain_AcquireImage();
        vkrMemSys_Update();
        vkrCmdFlush();
    }

    // system update
    {
        vkrSampler_Update();
        vkrMeshSys_Update();
        vkrImSys_Flush();
        vkrCmdFlush();
    }

    // setup phase
    {
        vkrMainPass_Setup();
        vkrTexTable_Update();
        vkrBindings_Update();
        vkrCmdFlush();
    }

    // execute phase
    vkrMainPass_Execute();

    // present phase
    vkrSwapchain_Submit(vkrCmdGet_G());

    // background work
    {
        vkrUploadLightmaps();
        vkrImSys_Clear();
        vkrCmdFlush();
    }

    ProfileEnd(pm_update);
}

void vkrSys_Shutdown(void)
{
    if (g_vkr.inst)
    {
        vkrDevice_WaitIdle();

        LmPack_Del(LmPack_Get());

        vkrMainPass_Del();

        vkrImSys_Shutdown();
        vkrMeshSys_Shutdown();
        vkrTargets_Shutdown();
        vkrBindings_Shutdown();
        vkrTexTable_Shutdown();
        vkrSampler_Shutdown();
        vkrRenderPass_Clear();

        vkrMemSys_Finalize();

        vkrContext_Del(&g_vkr.context);
        vkrSwapchain_Del(&g_vkr.chain);
        vkrFramebuffer_Shutdown();
        vkrSys_WindowShutdown();
        vkrMemSys_Shutdown();
        vkrDevice_Shutdown(&g_vkr);
        vkrInstance_Shutdown(&g_vkr);
    }
}

u32 vkrGetSyncIndex(void)
{
    return g_vkr.chain.syncIndex;
}

u32 vkrGetPrevSyncIndex(void)
{
    return (vkrGetSyncIndex() + (R_ResourceSets - 1u)) % R_ResourceSets;
}

u32 vkrGetNextSyncIndex(void)
{
    return (vkrGetSyncIndex() + 1u) % R_ResourceSets;
}

u32 vkrGetSwapIndex(void)
{
    return g_vkr.chain.imageIndex;
}

u32 vkrGetFrameCount(void)
{
    return Time_FrameCount();
}

vkrQueue* vkrGetQueue(vkrQueueId queueId)
{
    ASSERT(queueId < NELEM(g_vkr.queues));
    return &g_vkr.queues[queueId];
}

bool vkrGetHdrEnabled(void)
{
    switch (g_vkr.chain.colorSpace)
    {
    default:
        return false;
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: // Rec2100 w/ PQ OETF
        return true;
    case VK_COLOR_SPACE_DOLBYVISION_EXT: // Rec2100 w/ PQ OETF
        return true;
    case VK_COLOR_SPACE_HDR10_HLG_EXT: // Rec2100 w/ HLG OETF
        return true;
    }
}

float vkrGetWhitepoint(void)
{
    return ConVar_GetFloat(&cv_r_whitepoint);
}

float vkrGetDisplayNitsMin(void)
{
    return ConVar_GetFloat(&cv_r_display_nits_min);
}

float vkrGetDisplayNitsMax(void)
{
    return ConVar_GetFloat(&cv_r_display_nits_max);
}

float vkrGetUiNits(void)
{
    return ConVar_GetFloat(&cv_r_ui_nits);
}

Colorspace vkrGetRenderColorspace(void)
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

Colorspace vkrGetDisplayColorspace(void)
{
    switch (g_vkr.chain.colorSpace)
    {
    default:
        return Colorspace_Rec709;
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: // Rec2100 w/ PQ OETF
        return Colorspace_Rec2020;
    case VK_COLOR_SPACE_DOLBYVISION_EXT: // Rec2100 w/ PQ OETF
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
