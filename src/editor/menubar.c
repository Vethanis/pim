#include "editor/menubar.h"
#include "common/profiler.h"
#include "rendering/r_window.h"
#include "common/cvar.h"
#include "rendering/render_system.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "rendering/drawable.h"
#include "assets/asset_system.h"
#include "audio/audio_system.h"
#include "ui/cimgui_ext.h"

// ----------------------------------------------------------------------------

typedef struct edwin_s
{
    const char* name;
    void(*Draw)(bool* pEnabled);
    bool enabled;
} edwin_t;

static edwin_t ms_windows[] =
{
    { "Assets", AssetSys_Gui },
    { "Audio", AudioSys_Gui },
    { "CVars", ConVar_Gui },
    { "Drawables", EntSys_Gui },
    { "Meshes", MeshSys_Gui },
    { "Profiler", ProfileSys_Gui },
    { "Renderer", RenderSys_Gui },
    { "Textures", TextureSys_Gui },
};

// ----------------------------------------------------------------------------

static void ShowMenuBar(void);
static void FileMenuBar(void);
static void EditMenuBar(void);
static void WindowMenuBar(void);
static void ShowWindows(void);

// ----------------------------------------------------------------------------

void MenuBar_Init(void)
{

}

ProfileMark(pm_update, MenuBar_Update)
void MenuBar_Update(void)
{
    ProfileBegin(pm_update);

    ShowMenuBar();
    ShowWindows();

    ProfileEnd(pm_update);
}

void MenuBar_Shutdown(void)
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
    if (igMenuItem_Bool("New", "Ctrl+N", false, true))
    {

    }
    if (igMenuItem_Bool("Open", "Ctrl+O", false, true))
    {

    }
    if (igMenuItem_Bool("Save", "Ctrl+S", false, true))
    {

    }
    if (igMenuItem_Bool("Save As", "Ctrl+Shift+S", false, true))
    {

    }
    if (igMenuItem_Bool("Close", "Ctrl+W", false, true))
    {

    }
    if (igMenuItem_Bool("Exit", "Esc", false, true))
    {
        Window_Close(true);
    }
}

static void EditMenuBar(void)
{
    if (igMenuItem_Bool("Undo", "Ctrl+Z", false, true))
    {

    }
    if (igMenuItem_Bool("Redo", "Ctrl+Shift+Z", false, true))
    {

    }
    if (igMenuItem_Bool("Cut", "Ctrl+X", false, true))
    {

    }
    if (igMenuItem_Bool("Copy", "Ctrl+C", false, true))
    {

    }
    if (igMenuItem_Bool("Paste", "Ctrl+V", false, true))
    {

    }
}

static void WindowMenuBar(void)
{
    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        igMenuItem_BoolPtr(ms_windows[i].name, "", &ms_windows[i].enabled, true);
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
