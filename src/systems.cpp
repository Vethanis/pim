#include "systems.h"

#include <sokol/sokol_app.h>

#include "time/time_system.h"
#include "audio/audio_system.h"
#include "input/input_system.h"
#include "rendering/render_system.h"
#include "components/ecs.h"

#include "common/macro.h"

#include "quake/packfile.h"

namespace Systems
{
    static Quake::PackAssets ms_assets;

    void Init()
    {
        Array<Quake::DPackFile> arena = {};
        Quake::AddGameDirectory("packs/id1", ms_assets, arena);
        arena.reset();

        TimeSystem::Init();
        Ecs::Init();
        RenderSystem::Init();
        InputSystem::Init();
        AudioSystem::Init();
    }

    void Update()
    {
        TimeSystem::Update();
        RenderSystem::Update();
        InputSystem::Update();
        AudioSystem::Update();

        TimeSystem::Visualize();
        RenderSystem::Visualize();
        InputSystem::Visualize();
        AudioSystem::Visualize();

        RenderSystem::FrameEnd();
    }

    void Shutdown()
    {
        AudioSystem::Shutdown();
        InputSystem::Shutdown();
        RenderSystem::Shutdown();
        Ecs::Shutdown();
        TimeSystem::Shutdown();

        ms_assets.Reset();
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
