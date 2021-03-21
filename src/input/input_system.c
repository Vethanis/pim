#include "input_system.h"
#include "ui/cimgui_impl_glfw.h"
#include "rendering/r_window.h"
#include <GLFW/glfw3.h>
#include "common/time.h"
#include "common/profiler.h"

static GLFWwindow* ms_focused;
static u8 ms_prevKeys[KeyCode_COUNT];
static u8 ms_keys[KeyCode_COUNT];
static u8 ms_prevButtons[GLFW_MOUSE_BUTTON_LAST + 1];
static u8 ms_buttons[GLFW_MOUSE_BUTTON_LAST + 1];
static float ms_prevaxis[MouseAxis_COUNT];
static float ms_axis[MouseAxis_COUNT];

static void Input_OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
static void Input_OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods);
static void Input_OnScroll(GLFWwindow* window, double xoffset, double yoffset);
static void Input_OnChar(GLFWwindow* window, u32 c);
static void Input_OnMove(GLFWwindow* window, double cursorX, double cursorY);
static void Input_OnFocus(GLFWwindow* window, i32 focused);

// ----------------------------------------------------------------------------

void InputSys_Init(void)
{

}

ProfileMark(pm_update, InputSys_Update)
void InputSys_Update(void)
{
    ProfileBegin(pm_update);

    for (i32 i = 0; i < NELEM(ms_keys); ++i)
    {
        ms_prevKeys[i] = ms_keys[i];
    }
    for (i32 i = 0; i < NELEM(ms_buttons); ++i)
    {
        ms_prevButtons[i] = ms_buttons[i];
    }
    for (i32 i = 0; i < NELEM(ms_axis); ++i)
    {
        ms_prevaxis[i] = ms_axis[i];
    }
    glfwPollEvents();

    ProfileEnd(pm_update);
}

void InputSys_Shutdown(void)
{
    ms_focused = NULL;
}

void Input_RegWindow(GLFWwindow* window)
{
    if (window)
    {
        i32 focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
        if (focused)
        {
            ms_focused = window;
        }
        glfwSetKeyCallback(window, Input_OnKey);
        glfwSetMouseButtonCallback(window, Input_OnClick);
        glfwSetScrollCallback(window, Input_OnScroll);
        glfwSetCharCallback(window, Input_OnChar);
        glfwSetCursorPosCallback(window, Input_OnMove);
        glfwSetWindowFocusCallback(window, Input_OnFocus);
    }
}

GLFWwindow* Input_GetFocus(void)
{
    return ms_focused;
}

void Input_SetFocus(GLFWwindow* window)
{
    if (window)
    {
        glfwFocusWindow(window);
    }
}

bool Input_IsCursorCaptured(GLFWwindow* window)
{
    i32 mode = 0;
    if (window)
    {
        mode = glfwGetInputMode(window, GLFW_CURSOR);
    }
    return mode == GLFW_CURSOR_DISABLED;
}

void Input_CaptureCursor(GLFWwindow* window, bool capture)
{
    if (window)
    {
        i32 mode = capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
        glfwSetInputMode(window, GLFW_CURSOR, mode);
    }
}

bool Input_GetKey(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return ms_keys[key];
}

bool Input_GetButton(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return ms_buttons[button];
}

bool Input_IsKeyDown(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return ms_keys[key] && !ms_prevKeys[key];
}

bool Input_IsButtonDown(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return ms_buttons[button] && !ms_prevButtons[button];
}

bool Input_IsKeyUp(KeyCode key)
{
    ASSERT(key > KeyCode_Invalid);
    ASSERT(key < KeyCode_COUNT);
    return !ms_keys[key] && ms_prevKeys[key];
}

bool Input_IsButtonUp(MouseButton button)
{
    ASSERT(button >= 0);
    ASSERT(button < NELEM(ms_buttons));
    return !ms_buttons[button] && ms_prevButtons[button];
}

float Input_GetAxis(MouseAxis axis)
{
    ASSERT(axis >= 0);
    ASSERT(axis < NELEM(ms_axis));
    return ms_axis[axis];
}

float Input_GetDeltaAxis(MouseAxis axis)
{
    ASSERT(axis >= 0);
    ASSERT(axis < NELEM(ms_axis));
    return ms_axis[axis] - ms_prevaxis[axis];
}

static void Input_OnKey(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    ms_keys[key] = action != GLFW_RELEASE ? 1 : 0;
    if (!Input_IsCursorCaptured(window))
    {
        igImplGlfw_KeyCallback(window, key, scancode, action, mods);
    }
}

static void Input_OnClick(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    ms_buttons[button] = action != GLFW_RELEASE ? 1 : 0;
    if (!Input_IsCursorCaptured(window))
    {
        igImplGlfw_MouseButtonCallback(window, button, action, mods);
    }
}

static void Input_OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    ms_axis[MouseAxis_ScrollX] += (float)xoffset;
    ms_axis[MouseAxis_ScrollY] += (float)yoffset;
    if (!Input_IsCursorCaptured(window))
    {
        igImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }
}

static void Input_OnChar(GLFWwindow* window, u32 c)
{
    if (!Input_IsCursorCaptured(window))
    {
        igImplGlfw_CharCallback(window, c);
    }
}

static void Input_OnMove(GLFWwindow* window, double cursorX, double cursorY)
{
    float x = 2.0f * (float)(cursorX / Window_Width()) - 1.0f;
    float y = -2.0f * (float)(cursorY / Window_Height()) + 1.0f;
    ms_axis[MouseAxis_X] = x;
    ms_axis[MouseAxis_Y] = y;
}

static void Input_OnFocus(GLFWwindow* window, i32 focused)
{
    if (focused)
    {
        ms_focused = window;
    }
}
