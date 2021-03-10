#include "ui/cimgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_glfw.h"

PIM_C_BEGIN

bool igImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_InitForOpenGL(window, install_callbacks);
}

bool igImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_InitForVulkan(window, install_callbacks);
}

void igImplGlfw_Shutdown(void)
{
    ImGui_ImplGlfw_Shutdown();
}

void igImplGlfw_NewFrame(void)
{
    ImGui_ImplGlfw_NewFrame();
}

void igImplGlfw_MouseButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void igImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void igImplGlfw_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void igImplGlfw_CharCallback(GLFWwindow* window, u32 c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

PIM_C_END

// cimgui incorrectly links with these when IMGUI_DISABLE_DEMO_WINDOWS is defined
#if defined(IMGUI_DISABLE_DEMO_WINDOWS)

namespace ImGui
{
    bool ShowStyleSelector(char const *) { return false; }
    void ShowFontSelector(char const *) {}
    void ShowDemoWindow(bool *) {}
    void ShowUserGuide(void) {}
    void ShowStyleEditor(struct ImGuiStyle *) {}
    void ShowAboutWindow(bool *) {}
};

#endif // IMGUI_DISABLE_DEMO_WINDOWS
