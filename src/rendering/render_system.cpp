#include "rendering/render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>
#include "ui/imgui.h"

#include "common/int_types.h"
#include "containers/array.h"
#include "time/time_system.h"

#include "components/ecs.h"
#include "components/transform.h"
#include "rendering/components.h"

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
            ImGui::SetAllocatorFunctions(Allocator::ImGuiAllocFn, Allocator::ImGuiFreeFn);
            simgui_desc_t desc = {};
            simgui_setup(&desc);
        }
        ms_width = sapp_width();
        ms_height = sapp_height();

        Entity map = Ecs::Create({ LocalToWorld::Id, Drawable::Id });
    }

    void Update()
    {
        ms_width = sapp_width();
        ms_height = sapp_height();
        simgui_new_frame(ms_width, ms_height, TimeSystem::DeltaTimeF32());

        for (Entity entity : Ecs::Search({ { Drawable::Id, LocalToWorld::Id }, {}, {} }))
        {
            LocalToWorld& l2w = Ecs::Get<LocalToWorld>(entity);
            Drawable& drawable = Ecs::Get<Drawable>(entity);
        }
    }

    void Shutdown()
    {
        simgui_shutdown();
        sg_shutdown();
    }

    void FrameEnd()
    {
        sg_begin_default_pass(&ms_clear, ms_width, ms_height);
        {
            simgui_render();
        }
        sg_end_pass();

        sg_commit();
    }

    bool OnEvent(const sapp_event* evt)
    {
        return simgui_handle_event(evt);
    }

    void Visualize()
    {

    }
};
