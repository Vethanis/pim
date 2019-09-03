#include "render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>

#include "common/int_types.h"
#include "containers/array.h"
#include "common/stringutil.h"
#include "systems/time_system.h"
#include "os/fs.h"

#include <stdio.h>

namespace RenderSystem
{
    constexpr sg_pass_action ms_clear = 
    {
        0,
        {
            {
                SG_ACTION_CLEAR,
                { 0.25f, 0.25f, 0.5f, 0.0f }
            },
        },
    };

    static i32 ms_width;
    static i32 ms_height;

    static void LoadShaders()
    {
        Fs::Results results;
        results.Init(Allocator_Malloc);
        Fs::Find(ROOT_DIR"/src/shaders/*", results);
        for (const Fs::Info& file : results.files)
        {
            if (EndsWithStr(file.extension, ".hlsl"))
            {
                printf("Loading shader %s\n", file.path);
            }
        }
        results.Reset();
    }

    void Init()
    {
        {
            sg_desc desc = {};
            desc.mtl_device = sapp_metal_get_device();
            desc.mtl_drawable_cb = sapp_metal_get_drawable;
            desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
            desc.d3d11_device = sapp_d3d11_get_device();
            desc.d3d11_device_context = sapp_d3d11_get_device_context();
            desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
            desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
            sg_setup(&desc);
        }
        {
            simgui_desc_t desc = {};
            simgui_setup(&desc);
        }
        ms_width = sapp_width();
        ms_height = sapp_height();

        LoadShaders();
    }
    void Shutdown()
    {
        simgui_shutdown();
        sg_shutdown();
    }

    void BeginDraws()
    {
        ms_width = sapp_width();
        ms_height = sapp_height();
        sg_begin_default_pass(&ms_clear, ms_width, ms_height);
    }
    void EndDraws()
    {
        sg_end_pass();
        sg_commit();
    }

    void BeginVisualize()
    {
        simgui_new_frame(ms_width, ms_height, TimeSystem::DeltaTimeF32());
    }
    void EndVisualize()
    {
        simgui_render();
    }

    bool OnEvent(const sapp_event* evt)
    {
        return simgui_handle_event(evt);
    }

    void Visualize()
    {

    }
};