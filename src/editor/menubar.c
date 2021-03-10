#include "editor/menubar.h"
#include "common/profiler.h"
#include "rendering/r_window.h"
#include "common/cvar.h"
#include "rendering/render_system.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "assets/asset_system.h"
#include "audio/audio_system.h"
#include "ui/cimgui_ext.h"

// ----------------------------------------------------------------------------

typedef struct edwin_s
{
    const char* name;
    bool enabled;
    void(*Draw)(bool* pEnabled);
} edwin_t;

// ----------------------------------------------------------------------------

static void ShowMenuBar(void);
static void FileMenuBar(void);
static void EditMenuBar(void);
static void WindowMenuBar(void);
static void ShowWindows(void);

// ----------------------------------------------------------------------------

static edwin_t ms_windows[] =
{
    { "CVars", false, cvar_gui },
    { "Assets", false, asset_gui },
    { "Audio", false, audio_sys_ongui },
    { "Profiler", false, profile_gui },
    { "Renderer", false, render_sys_gui },
    { "Textures", false, texture_sys_gui },
    { "Meshes", false, mesh_sys_gui },
    { "ImGui Demo", false, igShowDemoWindow },
};

// ----------------------------------------------------------------------------

void menubar_init(void)
{

}

ProfileMark(pm_update, menubar_update)
void menubar_update(void)
{
    ProfileBegin(pm_update);

    ShowMenuBar();
    ShowWindows();

    ProfileEnd(pm_update);
}

void menubar_shutdown(void)
{

}

// ----------------------------------------------------------------------------

static void ShowMenuBar(void)
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
    if (igMenuItemBool("New", "Ctrl+N", false, true))
    {

    }
    if (igMenuItemBool("Open", "Ctrl+O", false, true))
    {

    }
    if (igMenuItemBool("Save", "Ctrl+S", false, true))
    {

    }
    if (igMenuItemBool("Save As", "Ctrl+Shift+S", false, true))
    {

    }
    if (igMenuItemBool("Close", "Ctrl+W", false, true))
    {

    }
    if (igMenuItemBool("Exit", "Esc", false, true))
    {
        window_close(true);
    }
}

static void EditMenuBar(void)
{
    if (igMenuItemBool("Undo", "Ctrl+Z", false, true))
    {

    }
    if (igMenuItemBool("Redo", "Ctrl+Shift+Z", false, true))
    {

    }
    if (igMenuItemBool("Cut", "Ctrl+X", false, true))
    {

    }
    if (igMenuItemBool("Copy", "Ctrl+C", false, true))
    {

    }
    if (igMenuItemBool("Paste", "Ctrl+V", false, true))
    {

    }
}

static void WindowMenuBar(void)
{
    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        igMenuItemBoolPtr(ms_windows[i].name, "", &ms_windows[i].enabled, true);
    }
}

static void ShowWindows(void)
{
    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        if (ms_windows[i].enabled)
        {
            ms_windows[i].Draw(&ms_windows[i].enabled);
        }
    }
}
