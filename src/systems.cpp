#include "systems.h"

#include <sokol/sokol_app.h>

#include "time/time_system.h"
#include "allocator/allocator.h"
#include "components/system.h"
#include "input/input_system.h"
#include "rendering/render_system.h"

namespace Systems
{
    void Init()
    {
        TimeSystem::Init();
        Allocator::Init();
        SystemRegistry::Init();
    }

    void Update()
    {
        TimeSystem::Update();
        Allocator::Update();
        SystemRegistry::Update();
        RenderSystem::FrameEnd();
    }

    void Shutdown()
    {
        SystemRegistry::Shutdown();
        Allocator::Shutdown();
        TimeSystem::Shutdown();
    }

    void OnEvent(const sapp_event* evt)
    {
        InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
    }

    void Exit()
    {
        sapp_request_quit();
    }
};
