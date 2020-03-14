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

static constexpr const char* ms_moduleNames[] =
{
    "core_module",
    "allocator_module",
};

static const core_module_t* CoreModule;
static const allocator_module_t* AllocatorModule;

static constexpr i32 kNumModules = NELEM(ms_moduleNames);
static const void* ms_modules[kNumModules];
static Graph ms_moduleGraph;

template<typename T>
static const T* LoadModT(const char* name)
{
    const T* ptr = reinterpret_cast<const T*>(pimod_load(name));
    ASSERT(ptr);
    return ptr;
}

#define LoadMod(T) LoadModT<T##_t>(#T)

static void LoadModules(int32_t argc, char** argv)
{
    CoreModule = LoadMod(core_module);
    AllocatorModule = LoadMod(allocator_module);

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
