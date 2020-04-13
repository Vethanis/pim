// dear imgui: Platform Binding for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [x] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: 3 cursors types are missing from GLFW.
//  [X] Platform: Keyboard arrays indexed using GLFW_KEY_* codes, e.g. ImGui::IsKeyPressed(GLFW_KEY_SPACE).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// About GLSL version:
// The 'glsl_version' initialization parameter defaults to "#version 150" if NULL.
// Only override if your GL version doesn't handle this GLSL version. Keep NULL if unsure!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    #include <stdint.h>

    int32_t ImGui_ImplGlfw_InitForOpenGL(struct GLFWwindow* window, int32_t install_callbacks);
    int32_t ImGui_ImplGlfw_InitForVulkan(struct GLFWwindow* window, int32_t install_callbacks);
    void ImGui_ImplGlfw_Shutdown();
    void ImGui_ImplGlfw_NewFrame();

    // GLFW callbacks
    // - When calling Init with 'install_callbacks=true': GLFW callbacks will be installed for you. They will call user's previously installed callbacks, if any.
    // - When calling Init with 'install_callbacks=false': GLFW callbacks won't be installed. You will need to call those function yourself from your own GLFW callbacks.
    void ImGui_ImplGlfw_MouseButtonCallback(struct GLFWwindow* window, int32_t button, int32_t action, int32_t mods);
    void ImGui_ImplGlfw_ScrollCallback(struct GLFWwindow* window, double xoffset, double yoffset);
    void ImGui_ImplGlfw_KeyCallback(struct GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
    void ImGui_ImplGlfw_CharCallback(struct GLFWwindow* window, uint32_t c);

#ifdef __cplusplus
}
#endif
