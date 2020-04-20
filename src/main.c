#include "common/time.h"
#include "common/random.h"
#include "allocator/allocator.h"
#include "input/input_system.h"
#include "threading/task.h"
#include "components/ecs.h"
#include "rendering/render_system.h"
#include "audio/audio_system.h"
#include "assets/asset_system.h"
#include "rendering/window.h"
#include "ui/ui.h"
#include "os/socket.h"
#include "logic/logic.h"
#include "common/profiler.h"

static void Init(void);
static void Update(void);
static void Shutdown(void);

int main()
{
    Init();
    while (window_is_open())
    {
        Update();
    }
    Shutdown();
    return 0;
}

static void Init(void)
{
    time_sys_init();            // setup sokol time
    alloc_sys_init();           // preallocate pools
    profile_sys_init();         // noop
    window_sys_init();          // gl context, window
    task_sys_init();            // enable async work
    ecs_sys_init();             // place to store data
    asset_sys_init();           // means of loading data
    network_sys_init();         // setup sockets
    render_sys_init();          // setup rendering resources
    audio_sys_init();           // setup audio callback
    input_sys_init();           // setup glfw input callbacks
    ui_sys_init();              // setup imgui
    logic_sys_init();           // setup game logic
}

ProfileMark(pm_update, Update)
static void Update(void)
{
    time_sys_update();          // update frame time + frame id
    ProfileBegin(pm_update);
    alloc_sys_update();         // reset linear allocator
    input_sys_update();         // pump input events to callbacks
    window_sys_update();        // update window size
    ui_sys_beginframe();        // ImGui::BeginFrame
    profile_sys_gui();          // display profile stats
    network_sys_update();       // transmit and receive game state
    logic_sys_update();         // update game simulation
    ecs_sys_update();           // noop
    asset_sys_update();         // stream assets
    render_sys_update();        // begin draw tasks
    audio_sys_update();         // handle audio events
    render_sys_present();       // clear framebuffer, draw results
    ui_sys_endframe();          // ImGui::EndFrame
    task_sys_update();          // schedule tasks
    window_swapbuffers();       // glfwSwapBuffers
    ProfileEnd(pm_update);
}

static void Shutdown(void)
{
    logic_sys_shutdown();
    ui_sys_shutdown();
    input_sys_shutdown();
    audio_sys_shutdown();
    render_sys_shutdown();
    network_sys_shutdown();
    asset_sys_shutdown();
    ecs_sys_shutdown();
    task_sys_shutdown();
    window_sys_shutdown();
    profile_sys_shutdown();
    alloc_sys_shutdown();
    time_sys_shutdown();
}
