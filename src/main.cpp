#include <sokol/sokol_app.h>

#include "common/module.h"
#include "containers/graph.h"

#include "common/time.h"
#include "allocator/allocator.h"
#include "components/system.h"
#include "input/input_system.h"
#include "common/random.h"
#include "rendering/render_system.h"

static constexpr const char* ms_moduleNames[] =
{
    "core_module",
};

static constexpr i32 kNumModules = NELEM(ms_moduleNames);
static pimod_t ms_modules[kNumModules];
static Graph ms_moduleGraph;

static i32 FindModule(cstr name)
{
    const pimod_t* const modules = ms_modules;
    for (i32 i = 0; i < kNumModules; ++i)
    {
        if (strcmp(name, modules[i].name) == 0)
        {
            return i;
        }
    }
    return -1;
}

static void LoadModules()
{
    for (i32 i = 0; i < kNumModules; ++i)
    {
        pim_err_t err = pimod_load(ms_moduleNames[i], ms_modules + i);
        ASSERT(err == PIM_ERR_OK);
    }
}

static void UnloadModules()
{
    for (i32 i = 0; i < kNumModules; ++i)
    {
        pimod_unload(ms_moduleNames[i]);
    }
}

static void InitModules()
{
    ms_moduleGraph.Init();
    for (i32 i = 0; i < kNumModules; ++i)
    {
        ms_moduleGraph.AddVertex();
    }

    for (i32 dst = 0; dst < kNumModules; ++dst)
    {
        const pimod_t& mod = ms_modules[dst];
        for (i32 j = 0; j < mod.import_count; ++j)
        {
            const i32 src = FindModule(mod.imports[j]);
            if (src == -1)
            {
                ASSERT(false);
                continue;
            }
            if (src == dst)
            {
                ASSERT(false);
                continue;
            }
            ms_moduleGraph.AddEdge(src, dst);
        }
    }

    ms_moduleGraph.Sort();

    for (i32 i = 0; i < kNumModules; ++i)
    {
        i32 j = ms_moduleGraph[i];
        pim_err_t err = ms_modules[j].init(ms_modules, kNumModules);
        ASSERT(err == PIM_ERR_OK);
    }
}

static void UpdateModules()
{
    for (i32 i = 0; i < kNumModules; ++i)
    {
        i32 j = ms_moduleGraph[i];
        pim_err_t err = ms_modules[j].update();
        ASSERT(err == PIM_ERR_OK);
    }
}

static void ShutdownModules()
{
    for (i32 i = kNumModules - 1; i >= 0; --i)
    {
        i32 j = ms_moduleGraph[i];
        pim_err_t err = ms_modules[j].shutdown();
        ASSERT(err == PIM_ERR_OK);
    }
    ms_moduleGraph.Reset();
}

static void Init()
{
    Random::Seed();
    Systems::Init();

    InitModules();

    pimod_func_t func = pimod_find(ARGS(ms_modules), "core_module", "ExampleFn");
    ASSERT(func);
    func(0, 0);
}

static void Update()
{
    Time::Update();
    Allocator::Update();
    Systems::Update();

    UpdateModules();

    RenderSystem::FrameEnd();
}

static void Shutdown()
{
    ShutdownModules();

    Systems::Shutdown();
    Allocator::Shutdown();
    Time::Shutdown();

    UnloadModules();
}

static void OnEvent(const sapp_event* evt)
{
    InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
}

sapp_desc sokol_main(int, char**)
{
    LoadModules();

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
