#include "common/time.h"
#include "common/random.h"
#include "allocator/allocator.h"
#include "input/input_system.h"
#include "threading/task.h"
#include "rendering/render_system.h"
#include "audio/audio_system.h"
#include "assets/asset_system.h"
#include "rendering/r_window.h"
#include "ui/ui.h"
#include "os/socket.h"
#include "logic/logic.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "common/cmd.h"
#include "common/console.h"
#include "editor/editor.h"
#include "common/serialize.h"

static void Init(void);
static void Update(void);
static void Shutdown(void);
static void OnGui(void);

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
    ser_sys_init();
    window_sys_init();          // gl context, window
    cmd_sys_init();
    con_sys_init();
    task_sys_init();            // enable async work
    asset_sys_init();           // means of loading data
    network_sys_init();         // setup sockets
    render_sys_init();          // setup rendering resources
    audio_sys_init();           // setup audio callback
    input_sys_init();           // setup glfw input callbacks
    logic_sys_init();           // setup game logic
    editor_sys_init();
}

static void Shutdown(void)
{
    editor_sys_shutdown();
    logic_sys_shutdown();
    input_sys_shutdown();
    audio_sys_shutdown();
    render_sys_shutdown();
    network_sys_shutdown();
    asset_sys_shutdown();
    task_sys_shutdown();
    con_sys_shutdown();
    cmd_sys_shutdown();
    window_sys_shutdown();
    ser_sys_shutdown();
    alloc_sys_shutdown();
    time_sys_shutdown();
}

ProfileMark(pm_input, InitPhase)
static void InitPhase(void)
{
    ProfileBegin(pm_input);
    input_sys_update();         // pump input events to callbacks
    window_sys_update();        // update window size
    network_sys_update();       // transmit and receive game state
    asset_sys_update();         // stream assets in
    ProfileEnd(pm_input);
}

ProfileMark(pm_simulate, SimulatePhase)
static void SimulatePhase(void)
{
    ProfileBegin(pm_simulate);
    cmd_sys_update();
    logic_sys_update();         // update game simulation
    task_sys_update();          // schedule tasks
    ProfileEnd(pm_simulate);
}

ProfileMark(pm_present, PresentPhase)
static void PresentPhase(void)
{
    ProfileBegin(pm_present);
    render_sys_update();        // draw tasks
    audio_sys_update();         // handle audio events
    ProfileEnd(pm_present);
}

ProfileMark(pm_update, Update)
static void Update(void)
{
    time_sys_update();          // bump frame id for profiler
    alloc_sys_update();         // reset linear allocator
    ProfileBegin(pm_update);

    InitPhase();
    SimulatePhase();
    OnGui();
    PresentPhase();

    window_swapbuffers();       // glfwSwapBuffers
    ProfileEnd(pm_update);
}

ProfileMark(pm_gui, OnGui)
static void OnGui(void)
{
    ProfileBegin(pm_gui);
    ui_sys_beginframe();        // ImGui::BeginFrame

    con_sys_update();
    editor_sys_update();

    ui_sys_endframe();          // ImGui::EndFrame
    ProfileEnd(pm_gui);
}
