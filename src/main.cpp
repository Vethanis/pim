#include <sokol/sokol_app.h>

#include "common/module.h"
#include "containers/graph.h"

#include "common/time.h"
#include "allocator/allocator.h"
#include "components/system.h"
#include "input/input_system.h"
#include "common/random.h"
#include "rendering/render_system.h"

#include "../core_module/core_module.h"

static core_module_t CoreModule;

static void LoadModules(int32_t argc, char** argv)
{
    if (!pimod_get("core_module", &CoreModule, sizeof(CoreModule)))
    {
        ASSERT(0);
    }
    CoreModule.Init(argc, argv);
}

static void UnloadModules()
{
    CoreModule.Shutdown();
    pimod_release("core_module");
}

static void Init()
{
    Random::Seed();
    Systems::Init();
}

static void Update()
{
    CoreModule.Update();

    Time::Update();
    Allocator::Update();
    Systems::Update();

    RenderSystem::FrameEnd();
}

static void Shutdown()
{
    Systems::Shutdown();
    Allocator::Shutdown();
    Time::Shutdown();

    UnloadModules();
}

static void OnEvent(const sapp_event* evt)
{
    InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
}

sapp_desc sokol_main(int argc, char* argv[])
{
    Time::Init();
    Allocator::Init();

    LoadModules(argc, argv);

    sapp_desc desc = {};
    desc.window_title = "Pim";
    desc.width = 320 * 4;
    desc.height = 240 * 4;
    desc.sample_count = 1;
    desc.swap_interval = 1;
    desc.alpha = false;
    desc.high_dpi = false;
    desc.init_cb = Init;
    desc.frame_cb = Update;
    desc.cleanup_cb = Shutdown;
    desc.event_cb = OnEvent;

    return desc;
}
