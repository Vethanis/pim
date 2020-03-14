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
#include "../allocator_module/allocator_module.h"

static const core_module_t* CoreModule;
static const allocator_module_t* AllocatorModule;

#define LoadMod(T) (const T##_t*)pimod_load(#T)

static void LoadModules(int32_t argc, char** argv)
{
    CoreModule = LoadMod(core_module);
    ASSERT(CoreModule);
    AllocatorModule = LoadMod(allocator_module);
    ASSERT(AllocatorModule);

    CoreModule->Init(argc, argv);
    constexpr int32_t sizes[EAlloc_Count] =
    {
        1,
        1 << 10,
        1 << 10,
        1 << 10,
    };
    AllocatorModule->Init(sizes, EAlloc_Count);
}

static void UnloadModules()
{
    AllocatorModule->Shutdown();
    CoreModule->Shutdown();

    pimod_unload("allocator_module");
    pimod_unload("core_module");
}

static void Init()
{
    Random::Seed();
    Systems::Init();
}

static void Update()
{
    CoreModule->Update();
    AllocatorModule->Update();

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
    LoadModules(argc, argv);

    Time::Init();
    Allocator::Init();

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
