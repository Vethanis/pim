#include "systems.h"

#include <sokol/sokol_app.h>

#include "common/time.h"
#include "allocator/allocator.h"
#include "components/system.h"
#include "input/input_system.h"
#include "common/random.h"
#include "rendering/render_system.h"

namespace Systems
{
    void Init()
    {
        Time::Init();
        Random::Seed();
        Allocator::Init();
        SystemRegistry::Init();
    }

    void Update()
    {
        Time::Update();
        Allocator::Update();
        SystemRegistry::Update();
        RenderSystem::FrameEnd();
    }

    void Shutdown()
    {
        SystemRegistry::Shutdown();
        Allocator::Shutdown();
        Time::Shutdown();
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
