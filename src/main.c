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
#include "common/cvars.h"
#include "common/cmd.h"
#include "common/console.h"
#include "editor/editor.h"
#include "common/serialize.h"
#include "script/script.h"

static bool Init(void);
static void Update(void);
static void Shutdown(void);
static void OnGui(void);

int main()
{
    if (!Init())
    {
        return -1;
    }
    while (Window_IsOpen())
    {
        Update();
    }
    Shutdown();
    return 0;
}

static bool Init(void)
{
    TimeSys_Init();
    MemSys_Init();
    ConVars_RegisterAll();
    SerSys_Init();
    WinSys_Init();
    cmd_sys_init();
    ConSys_Init();
    TaskSys_Init();
    AssetSys_Init();
    NetSys_Init();
    if (!RenderSys_Init())
    {
        return false;
    }
    AudioSys_Init();
    InputSys_Init();
    LogicSys_Init();
    EditorSys_Init();
    ScriptSys_Init();

    return true;
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
    cmd_sys_shutdown();
    WinSys_Shutdown();
    SerSys_Shutdown();
    MemSys_Shutdown();
    TimeSys_Shutdown();
}

ProfileMark(pm_update, Update)
static void Update(void)
{
    TimeSys_Update();           // bump frame id for profiler
    MemSys_Update();            // reset linear allocator
    ProfileBegin(pm_update);

    InputSys_Update();          // pump input events to callbacks
    NetSys_Update();            // transmit and receive game state
    AssetSys_Update();          // stream assets in
    TaskSys_Update();           // schedule tasks
    cmd_sys_update();           // execute console commands

    if (RenderSys_WindowUpdate())
    {
        LogicSys_Update();      // update game simulation
        OnGui();
        RenderSys_Update();
        AudioSys_Update();
    }

    Window_Wait();              // wait for target fps
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
