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
#include "math/vec_funcs.h"

#include <stdio.h>

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

struct DrawTask final : ECS::ForEachTask
{
    CType<Drawable> m_drawable;
    CType<LocalToWorld> m_l2w;

    DrawTask() : ECS::ForEachTask(
        { CTypeOf<Drawable>(), CTypeOf<LocalToWorld>() },
        { })
    {}

    void OnEntity(Entity entity) final
    {
        const Drawable* drawable = m_drawable.Get(entity);
        LocalToWorld* l2w = m_l2w.Get(entity);
        ASSERT(drawable);
        ASSERT(l2w);
        l2w->Value.x.x = math::sin(Random::NextF32());
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

    struct System final : ISystem
    {
        System() : ISystem("RenderSystem", { "InputSystem", "IEntitySystem", "ECS", }) {}

        DrawTask m_task;
        i32 m_frame;

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

            CTypeOf<Camera>();

            for (i32 i = 0; i < 1000000; ++i)
            {
                Entity entity = ECS::Create();
                if (i & 1)
                {
                    ECS::Add<Drawable>(entity);
                }
                if (i & 7)
                {
                    ECS::Add<LocalToWorld>(entity);
                }
                if (i == 0)
                {
                    ECS::Add<Camera>(entity);
                }
            }

            m_frame = 0;
        }

        void Update() final
        {
            Screen::Update();
            simgui_new_frame(Screen::Width(), Screen::Height(), Time::DeltaTimeF32());
            sg_begin_default_pass(&ms_clear, Screen::Width(), Screen::Height());

            u64 a = Time::Now();

            TaskSystem::Await(&m_task);
            m_task.Setup();
            TaskSystem::Submit(&m_task);
            //TaskSystem::Await(&m_task);

            ++m_frame;
            if ((m_frame & 63) == 0)
            {
                printf("dt: %f\n", Time::ToMilliseconds(Time::Now() - a));
            }
        }

        void Shutdown() final
        {
            TaskSystem::Await(&m_task);

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
