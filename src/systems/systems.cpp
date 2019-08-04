#include "systems.h"

#include "audio_system.h"
#include "ctrl_system.h"
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
        CtrlSystem::Init();
        EntitySystem::Init();
    }

    void Update()
    {
        const float dt = TimeSystem::Update();
        RenderSystem::NewFrame(dt);
        CtrlSystem::Update(dt);

        EntitySystem::Update(dt);

        AudioSystem::Update(dt);
        RenderSystem::Update(dt);
    }

    void Shutdown()
    {
        CtrlSystem::Shutdown();
        EntitySystem::Shutdown();
        RenderSystem::Shutdown();
        AudioSystem::Shutdown();
        TimeSystem::Shutdown();
    }

    void OnEvent(const sapp_event* evt)
    {
        CtrlSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
    }
};
