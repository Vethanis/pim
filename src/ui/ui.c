#include "ui/ui.h"
#include "ui/cimgui_ext.h"
#include "ui/cimgui_impl_glfw.h"
#include "allocator/allocator.h"
#include "rendering/r_window.h"
#include "common/profiler.h"
#include "common/cvars.h"
#include "common/stringutil.h"
#include "common/time.h"
#include "math/color.h"

static ImGuiContext* ms_ctx;

static void* ImGuiAllocFn(usize sz, void* userData)
{
    return Perm_Alloc((i32)sz);
}

static void ImGuiFreeFn(void* ptr, void* userData)
{
    Mem_Free(ptr);
}

static ImVec4 VEC_CALL BytesToColor(u32 r, u32 g, u32 b)
{
    R8G8B8A8_t c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = 0xff;
    float4 lin = Color_SDRToScene(GammaDecode_rgba8(c));
    lin.w = 0.95f;
    return *(ImVec4*)&lin;
}

static void SetupStyle(void)
{
    ImVec4 bgColor = BytesToColor(37, 37, 38);
    ImVec4 lightBgColor = BytesToColor(82, 82, 85);
    ImVec4 veryLightBgColor = BytesToColor(90, 90, 95);
    ImVec4 panelColor = BytesToColor(51, 51, 55);
    ImVec4 panelHoverColor = BytesToColor(29, 151, 236);
    ImVec4 panelActiveColor = BytesToColor(0, 119, 200);
    ImVec4 textColor = BytesToColor(255, 255, 255);
    ImVec4 textDisabledColor = BytesToColor(151, 151, 151);
    ImVec4 borderColor = BytesToColor(78, 78, 78);

    ImGuiStyle *const style = igGetStyle();
    igStyleColorsDark(style);
    ImVec4* colors = style->Colors;
    colors[ImGuiCol_Text] = textColor;
    colors[ImGuiCol_TextDisabled] = textDisabledColor;
    colors[ImGuiCol_TextSelectedBg] = bgColor;
    colors[ImGuiCol_WindowBg] = bgColor;
    colors[ImGuiCol_ChildBg] = bgColor;
    colors[ImGuiCol_PopupBg] = bgColor;
    colors[ImGuiCol_Border] = borderColor;
    colors[ImGuiCol_BorderShadow] = borderColor;
    colors[ImGuiCol_FrameBg] = panelColor;
    colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
    colors[ImGuiCol_FrameBgActive] = panelActiveColor;
    colors[ImGuiCol_TitleBg] = bgColor;
    colors[ImGuiCol_TitleBgActive] = bgColor;
    colors[ImGuiCol_TitleBgCollapsed] = bgColor;
    colors[ImGuiCol_MenuBarBg] = panelColor;
    colors[ImGuiCol_ScrollbarBg] = panelColor;
    colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
    colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
    colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
    colors[ImGuiCol_CheckMark] = panelActiveColor;
    colors[ImGuiCol_SliderGrab] = panelHoverColor;
    colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
    colors[ImGuiCol_Button] = panelColor;
    colors[ImGuiCol_ButtonHovered] = panelHoverColor;
    colors[ImGuiCol_ButtonActive] = panelHoverColor;
    colors[ImGuiCol_Header] = panelColor;
    colors[ImGuiCol_HeaderHovered] = panelHoverColor;
    colors[ImGuiCol_HeaderActive] = panelActiveColor;
    colors[ImGuiCol_Separator] = borderColor;
    colors[ImGuiCol_SeparatorHovered] = borderColor;
    colors[ImGuiCol_SeparatorActive] = borderColor;
    colors[ImGuiCol_ResizeGrip] = bgColor;
    colors[ImGuiCol_ResizeGripHovered] = panelColor;
    colors[ImGuiCol_ResizeGripActive] = lightBgColor;
    colors[ImGuiCol_PlotLines] = panelActiveColor;
    colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
    colors[ImGuiCol_PlotHistogram] = panelActiveColor;
    colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
    colors[ImGuiCol_DragDropTarget] = bgColor;
    colors[ImGuiCol_NavHighlight] = bgColor;
    colors[ImGuiCol_Tab] = bgColor;
    colors[ImGuiCol_TabActive] = panelActiveColor;
    colors[ImGuiCol_TabUnfocused] = bgColor;
    colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
    colors[ImGuiCol_TabHovered] = panelHoverColor;

    style->WindowRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->GrabRounding = 0.0f;
    style->PopupRounding = 0.0f;
    style->ScrollbarRounding = 0.0f;
    style->TabRounding = 0.0f;
}

static void UpdateOpacity(void)
{
    ImGuiStyle *const style = igGetStyle();
    ImVec4* pim_noalias colors = style->Colors;
    float opacity = ConVar_GetFloat(&cv_ui_opacity);
    for (i32 i = 0; i < NELEM(style->Colors); ++i)
    {
        colors[i].w = opacity;
    }
}

void UiSys_Init(GLFWwindow* window)
{
    ASSERT(window);

    ASSERT(igDebugCheckVersionAndDataLayout(
        "1.80", // cimgui does not export this! :(
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
    igImplGlfw_InitForVulkan(window, false);
    SetupStyle();
    UpdateOpacity();
}

ProfileMark(pm_beginframe, UiSys_BeginFrame)
void UiSys_BeginFrame(void)
{
    ProfileBegin(pm_beginframe);

    static u64 s_lap;
    if (ConVar_CheckDirty(&cv_ui_opacity, Time_Lap(&s_lap)))
    {
        UpdateOpacity();
    }

    igImplGlfw_NewFrame();
    igNewFrame();

    ProfileEnd(pm_beginframe);
}

ProfileMark(pm_endframe, UiSys_EndFrame)
void UiSys_EndFrame(void)
{
    ProfileBegin(pm_endframe);
    igEndFrame();
    ProfileEnd(pm_endframe);
}

void UiSys_Shutdown(void)
{
    igImplGlfw_Shutdown();
    igDestroyContext(ms_ctx);
    ms_ctx = NULL;
}
