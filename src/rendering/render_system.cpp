#include "rendering/render_system.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/util/sokol_imgui.h>
#include "ui/imgui.h"

#include "common/time.h"
#include "components/ecs.h"
#include "components/system.h"
#include "rendering/components.h"
#include "rendering/screen.h"
#include "components/transform.h"
#include "components/entity_system.h"
#include "math/vec_funcs.h"

namespace Screen
{
    static i32 ms_width;
    static i32 ms_height;

    i32 Width() { return ms_width; }
    i32 Height() { return ms_height; }

    static void Update()
    {
        Screen::ms_width = sapp_width();
        Screen::ms_height = sapp_height();
    }
};

namespace RenderSystem
{
    static constexpr sg_pass_action ms_clear =
    {
        0,
        {
            {
                SG_ACTION_CLEAR,
                { 0.25f, 0.25f, 0.5f, 0.0f }
            },
        },
    };

    struct DrawSystem final : IEntitySystem
    {
        DrawSystem() : IEntitySystem(
            "DrawSystem",
            {},
            { CTypeOf<Drawable>(false), CTypeOf<LocalToWorld>(true) },
            {})
        {}

        void Execute(Slice<const Entity> entities) final
        {
            for (Entity entity : entities)
            {
                const Drawable* drawable = ECS::Get<Drawable>(entity);
                LocalToWorld* l2w = ECS::Get<LocalToWorld>(entity);
                ASSERT(drawable);
                ASSERT(l2w);
                l2w->Value.x.x = math::sin(Random::NextF32());
            }
        }
    };

    static DrawSystem* ms_drawSystem;

    struct System final : ISystem
    {
        System() : ISystem("RenderSystem", { "InputSystem", "IEntitySystem", "ECS", }) {}

        void Init() final
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
            Screen::Update();

            for (i32 i = 0; i < 50000; ++i)
            {
                Entity entity = ECS::Create();
                bool added = ECS::Add<Drawable>(entity);
                ASSERT(added);
                added = ECS::Add<LocalToWorld>(entity);
                ASSERT(added);
            }

            ms_drawSystem = new DrawSystem();
        }

        void Update() final
        {
            Screen::Update();
            simgui_new_frame(Screen::Width(), Screen::Height(), Time::DeltaTimeF32());
            sg_begin_default_pass(&ms_clear, Screen::Width(), Screen::Height());
        }

        void Shutdown() final
        {
            delete ms_drawSystem;

            simgui_shutdown();
            sg_shutdown();
        }
    };

    static System ms_system;

    void FrameEnd()
    {
        simgui_render();
        sg_end_pass();
        sg_commit();
    }

    bool OnEvent(const sapp_event* evt)
    {
        return simgui_handle_event(evt);
    }
};
