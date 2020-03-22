#include <sokol/sokol_app.h>

#include "common/module.h"
#include "containers/graph.h"

#include "common/time.h"
#include "common/random.h"
#include "allocator/allocator.h"
#include "threading/task.h"

#include "components/system.h"
#include "input/input_system.h"
#include "rendering/render_system.h"

static void Init()
{
    Time::Init();
    Random::Seed();
    alloc_sys_init();
    task_sys_init();
    Systems::Init();
}

static void Update()
{
    Time::Update();
    alloc_sys_update();
    task_sys_update();
    Systems::Update();

    RenderSystem::FrameEnd();
}

static void Shutdown()
{
    Systems::Shutdown();
    task_sys_shutdown();
    alloc_sys_shutdown();
    Time::Shutdown();
}

static void OnEvent(const sapp_event* evt)
{
    InputSystem::OnEvent(evt, RenderSystem::OnEvent(evt));
}

sapp_desc sokol_main(int argc, char* argv[])
{
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
