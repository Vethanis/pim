#include "systems.h"

#include "audio_system.h"
#include "input_system.h"
#include "entity_system.h"
#include "render_system.h"
#include "time_system.h"

namespace Systems
{
    void Init()
    {
        TimeSystem::Init();
        AudioSystem::Init();
        RenderSystem::Init();
        InputSystem::Init();
        EntitySystem::Init();
    }

    void Update()
    {
        TimeSystem::Update();
        RenderSystem::BeginFrame();
        InputSystem::Update();

        EntitySystem::Update();

        AudioSystem::Update();
        RenderSystem::EndFrame();
    }

    void Shutdown()
    {
        InputSystem::Shutdown();
        EntitySystem::Shutdown();
        RenderSystem::Shutdown();
        AudioSystem::Shutdown();
        TimeSystem::Shutdown();
    }

    void OnEvent(const sapp_event* evt)
    {
        InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
    }
};
