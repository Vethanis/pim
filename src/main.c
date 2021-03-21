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
    while (Window_IsOpen())
    {
        Update();
    }
    Shutdown();
    return 0;
}

static void Init(void)
{
    TimeSys_Init();
    MemSys_Init();
    SerSys_Init();
    WinSys_Init();
    CmdSys_Init();
    ConSys_Init();
    TaskSys_Init();
    AssetSys_Init();
    NetSys_Init();
    RenderSys_Init();
    AudioSys_Init();
    InputSys_Init();
    LogicSys_Init();
    EditorSys_Init();
}

static void Shutdown(void)
{
    EditorSys_Shutdown();
    LogicSys_Shutdown();
    InputSys_Shutdown();
    AudioSys_Shutdown();
    RenderSys_Shutdown();
    NetSys_Shutdown();
    AssetSys_Shutdown();
    TaskSys_Shutdown();
    ConSys_Shutdown();
    CmdSys_Shutdown();
    WinSys_Shutdown();
    SerSys_Shutdown();
    MemSys_Shutdown();
    TimeSys_Shutdown();
}

ProfileMark(pm_input, InitPhase)
static void InitPhase(void)
{
    ProfileBegin(pm_input);
    InputSys_Update();         // pump input events to callbacks
    WinSys_Update();        // update window size
    NetSys_Update();       // transmit and receive game state
    AssetSys_Update();         // stream assets in
    ProfileEnd(pm_input);
}

ProfileMark(pm_simulate, SimulatePhase)
static void SimulatePhase(void)
{
    ProfileBegin(pm_simulate);
    CmdSys_Update();
    LogicSys_Update();         // update game simulation
    TaskSys_Update();          // schedule tasks
    ProfileEnd(pm_simulate);
}

ProfileMark(pm_present, PresentPhase)
static void PresentPhase(void)
{
    ProfileBegin(pm_present);
    RenderSys_Update();        // draw tasks
    AudioSys_Update();         // handle audio events
    ProfileEnd(pm_present);
}

ProfileMark(pm_update, Update)
static void Update(void)
{
    TimeSys_Update();          // bump frame id for profiler
    MemSys_Update();         // reset linear allocator
    ProfileBegin(pm_update);

    InitPhase();
    SimulatePhase();
    OnGui();
    PresentPhase();

    Window_SwapBuffers();       // wait for target fps
    ProfileEnd(pm_update);
}

ProfileMark(pm_gui, OnGui)
static void OnGui(void)
{
    ProfileBegin(pm_gui);
    UiSys_BeginFrame();        // ImGui::BeginFrame

    ConSys_Update();
    EditorSys_Update();

    UiSys_EndFrame();          // ImGui::EndFrame
    ProfileEnd(pm_gui);
}
