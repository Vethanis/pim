#include "systems.h"

#include "tables/tables.h"
#include "audio_system.h"
#include "input_system.h"
#include "entity_system.h"
#include "render_system.h"
#include "time_system.h"

namespace Systems
{
    static void Visualize();

    static void InputPhase()
    {
        TimeSystem::Update();
        InputSystem::Update();
    }
    static void SimulationPhase()
    {
        EntitySystem::Update();
    }
    static void PresentationPhase()
    {
        AudioSystem::Update();
        RenderSystem::BeginDraws();
        Visualize();
        RenderSystem::EndDraws();
    }

    void Init()
    {
        Tables::Init();
        TimeSystem::Init();
        InputSystem::Init();
        EntitySystem::Init();
        AudioSystem::Init();
        RenderSystem::Init();
    }

    void Update()
    {
        InputPhase();
        SimulationPhase();
        PresentationPhase();
    }

    void Shutdown()
    {
        RenderSystem::Shutdown();
        AudioSystem::Shutdown();
        EntitySystem::Shutdown();
        InputSystem::Shutdown();
        TimeSystem::Shutdown();
        Tables::Shutdown();
    }

    static void Visualize()
    {
        RenderSystem::BeginVisualize();
        Tables::Visualize();
        TimeSystem::Visualize();
        InputSystem::Visualize();
        EntitySystem::Visualize();
        RenderSystem::Visualize();
        AudioSystem::Visualize();
        RenderSystem::EndVisualize();
    }

    void OnEvent(const sapp_event* evt)
    {
        InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
    }
};
