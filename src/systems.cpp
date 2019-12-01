#include "systems.h"

#include <sokol/sokol_app.h>

#include "common/macro.h"
#include "time/time_system.h"
#include "allocator/allocator.h"
#include "components/ecs.h"
#include "assets/asset_system.h"
#include "audio/audio_system.h"
#include "input/input_system.h"
#include "rendering/render_system.h"

namespace Systems
{
    void Init()
    {
        TimeSystem::Init();
        Allocator::Init();
        Ecs::Init();
        InputSystem::Init();
        AudioSystem::Init();
        RenderSystem::Init();
        AssetSystem::Init();
    }

    void Update()
    {
        TimeSystem::Update();
        Allocator::Update();
        AssetSystem::Update();
        InputSystem::Update();
        AudioSystem::Update();

        RenderSystem::Update();

        TimeSystem::Visualize();
        RenderSystem::Visualize();
        InputSystem::Visualize();
        AudioSystem::Visualize();

        RenderSystem::FrameEnd();
    }

    void Shutdown()
    {
        RenderSystem::Shutdown();
        AudioSystem::Shutdown();
        InputSystem::Shutdown();
        Ecs::Shutdown();
        AssetSystem::Shutdown();
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
