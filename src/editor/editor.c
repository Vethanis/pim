#include "editor/editor.h"
#include "common/profiler.h"
#include "ui/cimgui.h"
#include "rendering/window.h"
#include "common/cvar.h"
#include "rendering/render_system.h"
#include "assets/asset_system.h"

typedef struct edwin_s
{
    const char* name;
    bool enabled;
    void(*fn)(void);
} edwin_t;

static void igDemoGui(void) { igShowDemoWindow(NULL); }

static edwin_t ms_windows[] =
{
    { "CVars", false, cvar_gui },
    { "Assets", false, asset_gui },
    { "Profiler", false, profile_gui },
    { "Renderer", false, render_sys_gui },
    { "ImGui Demo", false, igDemoGui },
};

// ----------------------------------------------------------------------------

static void MainMenuBar(void);
static void FileMenuBar(void);
static void EditMenuBar(void);
static void WindowMenuBar(void);
static void ShowWindows(void);

// ----------------------------------------------------------------------------

void editor_sys_init(void)
{

}

void editor_sys_update(void)
{
    MainMenuBar();
    ShowWindows();
}

void editor_sys_shutdown(void)
{

}

// ----------------------------------------------------------------------------

static void MainMenuBar(void)
{
    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            FileMenuBar();
            igEndMenu();
        }
        if (igBeginMenu("Edit", true))
        {
            EditMenuBar();
            igEndMenu();
        }
        if (igBeginMenu("Window", true))
        {
            WindowMenuBar();
            igEndMenu();
        }
        igEndMainMenuBar();
    }
}

static void FileMenuBar(void)
{
    if (igMenuItem("New"))
    {

    }
    if (igMenuItem("Open"))
    {

    }
    if (igMenuItem("Save"))
    {

    }
    if (igMenuItem("Save As"))
    {

    }
    if (igMenuItem("Close"))
    {

    }
    if (igMenuItem("Exit"))
    {
        window_close(true);
    }
}

static void EditMenuBar(void)
{
    if (igMenuItem("Undo"))
    {

    }
    if (igMenuItem("Redo"))
    {

    }
    if (igMenuItem("Cut"))
    {

    }
    if (igMenuItem("Copy"))
    {

    }
    if (igMenuItem("Paste"))
    {

    }
}

static void WindowMenuBar(void)
{
    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        if (igMenuItem(ms_windows[i].name))
        {
            ms_windows[i].enabled = !ms_windows[i].enabled;
        }
    }
}

ProfileMark(pm_showWindows, ShowWindows)
static void ShowWindows(void)
{
    ProfileBegin(pm_showWindows);

    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        if (ms_windows[i].enabled)
        {
            ms_windows[i].fn();
        }
    }

    ProfileEnd(pm_showWindows);
}
