#include "ui/ui.h"
#include "ui/cimgui.h"
#include "ui/imgui_impl_glfw.h"
#include "ui/imgui_impl_opengl3.h"
#include "allocator/allocator.h"
#include "rendering/window.h"
#include "common/profiler.h"
#include "common/cvar.h"

static ImGuiContext* ms_ctx;

static void* ImGuiAllocFn(usize sz, void* userData) { return perm_malloc((i32)sz); }
static void ImGuiFreeFn(void* ptr, void* userData) { pim_free(ptr); }

void ui_sys_init(void)
{
    ASSERT(igDebugCheckVersionAndDataLayout(
        CIMGUI_VERSION,
        sizeof(ImGuiIO),
        sizeof(ImGuiStyle),
        sizeof(ImVec2),
        sizeof(ImVec4),
        sizeof(ImDrawVert),
        sizeof(ImDrawIdx)));
    igSetAllocatorFunctions(ImGuiAllocFn, ImGuiFreeFn, NULL);
    ms_ctx = igCreateContext(NULL);
    ASSERT(ms_ctx);
    igSetCurrentContext(ms_ctx);
    igStyleColorsDark(NULL);
    ImGui_ImplGlfw_InitForOpenGL(window_ptr(), false);
    ImGui_ImplOpenGL3_Init();
}

ProfileMark(pm_beginframe, ui_sys_beginframe)
void ui_sys_beginframe(void)
{
    ProfileBegin(pm_beginframe);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();

    ProfileEnd(pm_beginframe);
}

ProfileMark(pm_endframe, ui_sys_endframe)
void ui_sys_endframe(void)
{
    ProfileBegin(pm_endframe);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

    ProfileEnd(pm_endframe);
}

void ui_sys_shutdown(void)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(ms_ctx);
    ms_ctx = NULL;
}
