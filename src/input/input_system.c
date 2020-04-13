#include "input_system.h"
#include "ui/imgui_impl_glfw.h"
#include "rendering/window.h"
#include <GLFW/glfw3.h>

static InputState ms_keys[KeyCode_COUNT];
static InputState ms_buttons[GLFW_MOUSE_BUTTON_LAST + 1];
static float ms_mouseX;
static float ms_mouseY;
static float ms_scrollV;
static float ms_scrollH;

static void OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
static void OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods);
static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
static void OnChar(GLFWwindow* window, u32 c);
static void OnMove(GLFWwindow* window, double cursorX, double cursorY);

// ----------------------------------------------------------------------------

void input_sys_init(void)
{
    GLFWwindow* window = window_ptr();
    ASSERT(window);
    glfwSetKeyCallback(window, OnKey);
    glfwSetMouseButtonCallback(window, OnClick);
    glfwSetScrollCallback(window, OnScroll);
    glfwSetCharCallback(window, OnChar);
    glfwSetCursorPosCallback(window, OnMove);
}

void input_sys_update(void)
{
    glfwPollEvents();
}

void input_sys_shutdown(void)
{

}

InputState input_get_key(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return ms_keys[key];
}

InputState input_get_button(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return ms_buttons[button];
}

float input_get_axis(MouseAxis axis)
{
    switch (axis)
    {
    case MouseAxis_X:
        return ms_mouseX;
    case MouseAxis_Y:
        return ms_mouseY;
    case MouseAxis_ScrollX:
        return ms_scrollH;
    case MouseAxis_ScrollY:
        return ms_scrollV;
    default:
        ASSERT(false);
        return 0.0f;
    }
}

static void OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    ms_keys[key] = action;
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

static void OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    ms_buttons[button] = action;
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

static void OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    ms_scrollH = (float)xoffset;
    ms_scrollV = (float)yoffset;
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

static void OnChar(GLFWwindow* window, u32 c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

static void OnMove(GLFWwindow* window, double cursorX, double cursorY)
{
    ms_mouseX = 2.0f * (float)(cursorX / window_width()) - 1.0f;
    ms_mouseY = -2.0f * (float)(cursorY / window_height()) + 1.0f;
}
