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
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mainpass.h"
#include "rendering/vulkan/vkr_imgui.h"
#include "rendering/vulkan/vkr_exposurepass.h"
#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/render_system.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/screenblit.h"
#include "rendering/lightmap.h"
#include "ui/cimgui.h"
#include "ui/ui.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/time.h"
#include "common/cvar.h"
#include "common/atomics.h"
#include "containers/table.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/quat_funcs.h"
#include "threading/task.h"
#include <string.h>

vkr_t g_vkr;

static cvar_t* cv_r_sun_dir;
static cvar_t* cv_r_sun_col;
static cvar_t* cv_r_sun_lum;

static cvar_t cv_lm_upload =
{
    .type = cvart_bool,
    .name = "lm_upload",
    .value = "0",
    .desc = "upload lightmap data to GPU",
};

bool vkr_init(i32 width, i32 height)
{
    memset(&g_vkr, 0, sizeof(g_vkr));

    cvar_reg(&cv_lm_upload);
    cv_r_sun_dir = cvar_find("r_sun_dir");
    cv_r_sun_col = cvar_find("r_sun_col");
    cv_r_sun_lum = cvar_find("r_sun_lum");
    ASSERT(cv_r_sun_dir);
    ASSERT(cv_r_sun_col);
    ASSERT(cv_r_sun_lum);

    bool success = true;

    if (!vkrInstance_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrDisplay_New(&g_vkr.display, width, height, "pimvk"))
    {
        success = false;
        goto cleanup;
    }
    ui_sys_init(g_vkr.display.window);

    if (!vkrDevice_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrAllocator_New(&g_vkr.allocator))
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

    const u32 nullColor = 0x0;
    if (!vkrTexture2D_New(
        &g_vkr.nullTexture,
        1, 1,
        VK_FORMAT_R8G8B8A8_SRGB,
        &nullColor, sizeof(nullColor)))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrMainPass_New(&g_vkr.mainPass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrExposurePass_New(&g_vkr.exposurePass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrImGuiPass_New(&g_vkr.imguiPass, g_vkr.mainPass.renderPass))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrScreenBlit_New(&g_vkr.screenBlit))
    {
        success = false;
        goto cleanup;
    }

    vkrSwapchain_SetupBuffers(&g_vkr.chain, g_vkr.mainPass.renderPass);

cleanup:

    if (!success)
    {
        vkr_shutdown();
    }
    return success;
}

static void vkrUploadLightmaps(void)
{
    lmpack_t* pack = lmpack_get();
    for (i32 i = 0; i < pack->lmCount; ++i)
    {
        lightmap_t* lm = pack->lightmaps + i;
        lightmap_upload(lm);
    }
}

ProfileMark(pm_update, vkr_update)
void vkr_update(void)
{
    if (!g_vkr.inst)
    {
        return;
    }

    if (!vkrDisplay_IsOpen(&g_vkr.display))
    {
        return;
    }

    if (vkrDisplay_UpdateSize(&g_vkr.display))
    {
        vkrSwapchain_Recreate(
            &g_vkr.chain,
            &g_vkr.display,
            g_vkr.mainPass.renderPass);
    }
    vkrSwapchain* chain = &g_vkr.chain;
    if (!chain->handle)
    {
        return;
    }

    ProfileBegin(pm_update);

    VkFence fence = NULL;
    VkCommandBuffer cmd = NULL;
    vkrSwapchain_AcquireSync(chain, &cmd, &fence);
    vkrAllocator_Update(&g_vkr.allocator);
    vkrCmdBegin(cmd);
    vkrMainPass_Draw(&g_vkr.mainPass, cmd, fence);
    vkrCmdEnd(cmd);
    vkrSwapchain_Submit(chain, cmd);
    vkrExposurePass_Execute(&g_vkr.exposurePass);
    vkrSwapchain_Present(chain);

    if (cvar_get_bool(&cv_lm_upload))
    {
        cvar_set_bool(&cv_lm_upload, false);
        vkrUploadLightmaps();
    }

    ProfileEnd(pm_update);
}

void vkr_shutdown(void)
{
    if (g_vkr.inst)
    {
        vkrDevice_WaitIdle();

        mesh_sys_vkfree();
        vkrTexture2D_Del(&g_vkr.nullTexture);
        texture_sys_vkfree();
        lmpack_del(lmpack_get());

        vkrScreenBlit_Del(&g_vkr.screenBlit);
        ui_sys_shutdown();
        vkrImGuiPass_Del(&g_vkr.imguiPass);
        vkrExposurePass_Del(&g_vkr.exposurePass);
        vkrMainPass_Del(&g_vkr.mainPass);

        vkrAllocator_Finalize(&g_vkr.allocator);

        vkrContext_Del(&g_vkr.context);
        vkrSwapchain_Del(&g_vkr.chain);
        vkrAllocator_Del(&g_vkr.allocator);
        vkrDevice_Shutdown(&g_vkr);
        vkrDisplay_Del(&g_vkr.display);
        vkrInstance_Shutdown(&g_vkr);
    }
}

void vkr_onload(void)
{
    if (g_vkr.allocator.handle)
    {
        vkrAllocator_Update(&g_vkr.allocator);
    }
}

void vkr_onunload(void)
{
    if (g_vkr.allocator.handle)
    {
        vkrAllocator_Update(&g_vkr.allocator);
    }
}
