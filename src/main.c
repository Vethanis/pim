#include <sokol/sokol_app.h>

#include "common/time.h"
#include "common/random.h"
#include "allocator/allocator.h"
#include "input/input_system.h"
#include "threading/task.h"
#include "components/ecs.h"
#include "rendering/render_system.h"
#include "audio/audio_system.h"
#include "assets/asset_system.h"
#include "os/socket.h"

typedef struct system_s
{
    void(*Init)(void);
    void(*Update)(void);
    void(*Shutdown)(void);
} system_t;

static const system_t ms_systems[] =
{
    { time_sys_init, time_sys_update, time_sys_shutdown },
    { rand_sys_init, rand_sys_update, rand_sys_shutdown },
    { alloc_sys_init, alloc_sys_update, alloc_sys_shutdown },
    { input_sys_init, input_sys_update, input_sys_shutdown },
    { task_sys_init, task_sys_update, task_sys_shutdown },
    { ecs_sys_init, ecs_sys_update, ecs_sys_shutdown },
    { render_sys_init, render_sys_update, render_sys_shutdown },
    { audio_sys_init, audio_sys_update, audio_sys_shutdown },
    { asset_sys_init, asset_sys_update, asset_sys_shutdown },
    { network_sys_init, network_sys_update, network_sys_shutdown },
};

static void Init(void)
{
    for (int32_t i = 0; i < NELEM(ms_systems); ++i)
    {
        ms_systems[i].Init();
    }
}

static void Update(void)
{
    for (int32_t i = 0; i < NELEM(ms_systems); ++i)
    {
        ms_systems[i].Update();
    }
    input_sys_frameend();
    render_sys_frameend();
}

static void Shutdown(void)
{
    for (int32_t i = NELEM(ms_systems) - 1; i >= 0; --i)
    {
        ms_systems[i].Shutdown();
    }
}

static void OnEvent(const sapp_event* evt)
{
    input_sys_onevent(evt, render_sys_onevent(evt));
}

sapp_desc sokol_main(int argc, char* argv[])
{
    sapp_desc desc =
    {
        .window_title = "Pim",
        .width = 320 * 4,
        .height = 240 * 4,
        .sample_count = 1,
        .swap_interval = 1,
        .alpha = 0,
        .high_dpi = 0,
        .init_cb = Init,
        .frame_cb = Update,
        .cleanup_cb = Shutdown,
        .event_cb = OnEvent,
    };
    return desc;
}
