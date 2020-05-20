#include "input_system.h"
#include "ui/imgui_impl_glfw.h"
#include "rendering/window.h"
#include <GLFW/glfw3.h>
#include "common/time.h"
#include "common/profiler.h"

static u64 ms_frametick;
static u64 ms_keyTimes[KeyCode_COUNT];
static u8 ms_keys[KeyCode_COUNT];
static u64 ms_buttonTimes[GLFW_MOUSE_BUTTON_LAST + 1];
static u8 ms_buttons[GLFW_MOUSE_BUTTON_LAST + 1];
static float ms_prevaxis[MouseAxis_COUNT];
static float ms_axis[MouseAxis_COUNT];

static void OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
static void OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods);
static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
static void OnChar(GLFWwindow* window, u32 c);
static void OnMove(GLFWwindow* window, double cursorX, double cursorY);
static bool IsRecent(u64 tick) { return tick >= ms_frametick; }

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
    ms_frametick = time_now();
}

ProfileMark(pm_update, input_sys_update)
void input_sys_update(void)
{
    ProfileBegin(pm_update);

    ms_frametick = time_now();
    ms_prevaxis[MouseAxis_ScrollX] = ms_axis[MouseAxis_ScrollX];
    ms_prevaxis[MouseAxis_ScrollY] = ms_axis[MouseAxis_ScrollY];
    ms_prevaxis[MouseAxis_X] = ms_axis[MouseAxis_X];
    ms_prevaxis[MouseAxis_Y] = ms_axis[MouseAxis_Y];
    glfwPollEvents();

    ProfileEnd(pm_update);
}

void input_sys_shutdown(void)
{

}

bool input_key(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return ms_keys[key];
}

bool input_button(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return ms_buttons[button];
}

bool input_keydown(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return ms_keys[key] && IsRecent(ms_keyTimes[key]);
}

bool input_buttondown(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return ms_buttons[button] && IsRecent(ms_buttonTimes[button]);
}

bool input_keyup(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return !ms_keys[key] && IsRecent(ms_keyTimes[key]);
}

bool input_buttonup(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return !ms_buttons[button] && IsRecent(ms_buttonTimes[button]);
}

float input_axis(MouseAxis axis)
{
    ASSERT(axis >= 0);
    ASSERT(axis < NELEM(ms_axis));
    return ms_axis[axis];
}

float input_delta_axis(MouseAxis axis)
{
    ASSERT(axis >= 0);
    ASSERT(axis < NELEM(ms_axis));
    return ms_axis[axis] - ms_prevaxis[axis];
}

static void OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    u8 state = action != GLFW_RELEASE ? 1 : 0;
    if (ms_keys[key] != state)
    {
        ms_keys[key] = state;
        ms_keyTimes[key] = time_now();
    }
    if (!window_cursor_captured())
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    }
}

static void OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    u8 state = action != GLFW_RELEASE ? 1 : 0;
    if (ms_buttons[button] != state)
    {
        ms_buttons[button] = state;
        ms_buttonTimes[button] = time_now();
    }
    if (!window_cursor_captured())
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    }
}

static void OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    ms_axis[MouseAxis_ScrollX] += (float)xoffset;
    ms_axis[MouseAxis_ScrollY] += (float)yoffset;
    if (!window_cursor_captured())
    {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }
}

static void OnChar(GLFWwindow* window, u32 c)
{
    if (!window_cursor_captured())
    {
        ImGui_ImplGlfw_CharCallback(window, c);
    }
}

static void OnMove(GLFWwindow* window, double cursorX, double cursorY)
{
    float x = 2.0f * (float)(cursorX / window_width()) - 1.0f;
    float y = -2.0f * (float)(cursorY / window_height()) + 1.0f;
    ms_axis[MouseAxis_X] = x;
    ms_axis[MouseAxis_Y] = y;
}
