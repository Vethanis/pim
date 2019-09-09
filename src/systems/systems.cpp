#include "systems.h"

#include <sokol/sokol_app.h>

#include "systems/time_system.h"
#include "systems/audio_system.h"
#include "systems/input_system.h"
#include "systems/render_system.h"

#include "common/macro.h"

#include "quake/packfile.h"

namespace Systems
{
    static Quake::SearchPaths ms_paths;

    static System ms_systems[] =
    {
        TimeSystem::GetSystem(),
        RenderSystem::GetSystem(),
        InputSystem::GetSystem(),
        AudioSystem::GetSystem(),
    };

    void Init()
    {
        Array<Quake::DPackFile> arena = {};
        Quake::AddGameDirectory(PacksDir"/id1", ms_paths, arena);
        arena.reset();

        for (const System& system : ms_systems)
        {
            if (system.enabled)
            {
                system.Init();
            }
        }
    }

    void Update()
    {
        for (const System& system : ms_systems)
        {
            if (system.enabled)
            {
                system.Update();
            }
        }
        for (const System& system : ms_systems)
        {
            if (system.visualizing)
            {
                system.Visualize();
            }
        }
        RenderSystem::FrameEnd();
    }

    void Shutdown()
    {
        for (i32 i = CountOf(ms_systems) - 1; i >= 0; --i)
        {
            if (ms_systems[i].enabled)
            {
                ms_systems[i].Shutdown();
            }
        }
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
