#include "editor/menubar.h"
#include "common/profiler.h"
#include "rendering/r_window.h"
#include "common/cvar.h"
#include "rendering/render_system.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "assets/asset_system.h"
#include "audio/audio_system.h"
#include <imgui/imgui.h>

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
    { "ImGui Demo", false, ImGui::ShowDemoWindow },
};

// ----------------------------------------------------------------------------

extern "C" void menubar_init(void)
{

}

ProfileMark(pm_update, menubar_update)
extern "C" void menubar_update(void)
{
    ProfileBegin(pm_update);

    ShowMenuBar();
    ShowWindows();

    ProfileEnd(pm_update);
}

extern "C" void menubar_shutdown(void)
{

}

// ----------------------------------------------------------------------------

static void ShowMenuBar(void)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File", true))
        {
            FileMenuBar();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit", true))
        {
            EditMenuBar();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window", true))
        {
            WindowMenuBar();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void FileMenuBar(void)
{
    if (ImGui::MenuItem("New"))
    {

    }
    if (ImGui::MenuItem("Open"))
    {

    }
    if (ImGui::MenuItem("Save"))
    {

    }
    if (ImGui::MenuItem("Save As"))
    {

    }
    if (ImGui::MenuItem("Close"))
    {

    }
    if (ImGui::MenuItem("Exit"))
    {
        window_close(true);
    }
}

static void EditMenuBar(void)
{
    if (ImGui::MenuItem("Undo"))
    {

    }
    if (ImGui::MenuItem("Redo"))
    {

    }
    if (ImGui::MenuItem("Cut"))
    {

    }
    if (ImGui::MenuItem("Copy"))
    {

    }
    if (ImGui::MenuItem("Paste"))
    {

    }
}

static void WindowMenuBar(void)
{
    for (i32 i = 0; i < NELEM(ms_windows); ++i)
    {
        ImGui::MenuItem(ms_windows[i].name, "", &ms_windows[i].enabled, true);
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
