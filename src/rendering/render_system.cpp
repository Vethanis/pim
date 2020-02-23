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
#include "threading/barrier_task.h"

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
    f32 m_iteration;
    f32 m_expect;
    f32 m_write;

    DrawTask() : ECS::ForEachTask(), m_iteration(0.0f) {}

    void Setup(f32 expect, f32 write)
    {
        ++m_iteration;
        m_expect = expect;
        m_write = write;
        SetQuery({ CTypeOf<Drawable>(), CTypeOf<LocalToWorld>() }, {});
    }

    void OnEntities(Slice<const Entity> entities) final
    {
        const f32 iteration = m_iteration;
        const f32 expect = m_expect;
        const f32 write = m_write;
        Slice<const Drawable> drawables = m_drawable.GetRow();
        Slice<LocalToWorld> l2ws = m_l2w.GetRow();

        for (Entity entity : entities)
        {
            ASSERT(ECS::Has<Drawable>(entity));
            ASSERT(ECS::Has<LocalToWorld>(entity));

            const Drawable& drawable = drawables[entity.index];
            LocalToWorld& l2w = l2ws[entity.index];

            float4& c0 = l2w.Value.x;
            float4& c1 = l2w.Value.y;

            const f32 index = (f32)entity.index;
            const f32 version = (f32)entity.version;

            if (iteration <= 1)
            {
                c0 = float4(iteration, index, version, 0.0f);
            }
            else
            {
                ASSERT(math::distance(c0.x, iteration) <= 1.0f);
                ASSERT(c0.y == index);
                ASSERT(c0.z == version);
                ASSERT(c1.y == expect);
            }
            c0.x = iteration;
            c0.w += 1.0f;
            c1.x = math::sin(Random::NextF32());
            c1.y = write;
        }
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
        System() : ISystem("RenderSystem", { "InputSystem", "ECS", "TaskSystem" }) {}

        DrawTask m_task1;
        BarrierTask m_barrier;
        DrawTask m_task2;

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

            constexpr i32 kThousand = 1000;
            constexpr i32 kMillion = 1000 * kThousand;

            constexpr i32 kCount = 1 * kMillion;
            for (i32 i = 0; i < kCount; ++i)
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
        }

        void Update() final
        {
            Screen::Update();
            simgui_new_frame(Screen::Width(), Screen::Height(), Time::DeltaTimeF32());
            sg_begin_default_pass(&ms_clear, Screen::Width(), Screen::Height());

            const u64 a = Time::Now();

            TaskSystem::Await(&m_task2);

            m_task1.Setup(math::Pi, math::Tau);
            m_barrier.Setup(&m_task1);
            m_task2.Setup(math::Tau, math::Pi);

            TaskSystem::Submit(&m_task1);
            TaskSystem::Submit(&m_barrier);
            TaskSystem::Submit(&m_task2);

            f32 ms = Time::ToMilliseconds(Time::Now() - a);
            static f32 s_avg = 0.0f;

            f32 alpha = 1.0f / Min((f32)Time::FrameCount(), 120.0f);
            s_avg = math::lerp(s_avg, ms, alpha);

            ImGui::Begin("RenderSystem");
            ImGui::Text("DrawTask Submit ms: %.2f", s_avg);
            ImGui::End();
        }

        void Shutdown() final
        {
            TaskSystem::Await(&m_task2);

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
