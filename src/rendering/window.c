#include "rendering/window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "io/fd.h"
#include "common/time.h"
#include "common/cvar.h"
#include "threading/intrin.h"
#include "threading/sleep.h"
#include "common/profiler.h"
#include <math.h>

static cvar_t cv_FpsLimit =
{
    "fps_limit",
    "125",
    "limits fps when above this value"
};

static GLFWwindow* ms_window;
static i32 ms_width;
static i32 ms_height;
static i32 ms_target;
static u64 ms_lastSwap;
static bool ms_cursorCaptured;

static GLFWwindow* CreateGlfwWindow(i32 width, i32 height, const char* title, bool fullscreen);
static void OnGlfwError(i32 error_code, const char* description);

// ----------------------------------------------------------------------------

void window_sys_init(void)
{
    cvar_reg(&cv_FpsLimit);
    ms_lastSwap = time_now();

    glfwSetErrorCallback(OnGlfwError);
    i32 rv = glfwInit();
    ASSERT(rv);

    ms_window = CreateGlfwWindow(800, 600, "Pim", true);
    ASSERT(ms_window);
    glfwMakeContextCurrent(ms_window);
    glfwSwapInterval(0);
    glfwGetWindowSize(ms_window, &ms_width, &ms_height);
    ms_cursorCaptured = false;

    rv = gladLoadGL();
    ASSERT(rv);
    ASSERT(!glGetError());
}

ProfileMark(pm_update, window_sys_update)
void window_sys_update(void)
{
    ProfileBegin(pm_update);

    ASSERT(ms_window);
    glfwGetWindowSize(ms_window, &ms_width, &ms_height);

    ProfileEnd(pm_update);
}

void window_sys_shutdown(void)
{
    ASSERT(ms_window);
    glfwDestroyWindow(ms_window);
    ms_window = NULL;
    glfwTerminate();
}

i32 window_width(void)
{
    ASSERT(ms_window);
    return ms_width;
}

i32 window_height(void)
{
    ASSERT(ms_window);
    return ms_height;
}

bool window_is_open(void)
{
    ASSERT(ms_window);
    return !glfwWindowShouldClose(ms_window);
}

void window_close(bool shouldClose)
{
    ASSERT(ms_window);
    glfwSetWindowShouldClose(ms_window, shouldClose);
}

float window_get_target(void)
{
    return cv_FpsLimit.asFloat;
}

void window_set_target(float fps)
{
    ASSERT(fps >= 1.0f);
    cvar_set_float(&cv_FpsLimit, fps);
}

ProfileMark(pm_waitfps, wait_for_target_fps)
static void wait_for_target_fps(void)
{
    ProfileBegin(pm_waitfps);

    const double targetMS = 1000.0 / cv_FpsLimit.asFloat;
    const double diffMS = targetMS - time_milli(time_now() - ms_lastSwap);
    if (diffMS >= 1.0)
    {
        intrin_sleep((u32)diffMS);
    }

    ms_lastSwap = time_now();

    ProfileEnd(pm_waitfps);
}

ProfileMark(pm_swapbuffers, window_swapbuffers)
void window_swapbuffers(void)
{
    ProfileBegin(pm_swapbuffers);

    ASSERT(ms_window);
    glfwSwapBuffers(ms_window);

    ProfileEnd(pm_swapbuffers);

    wait_for_target_fps();
}

bool window_cursor_captured(void)
{
    ASSERT(ms_window);
    return ms_cursorCaptured;
}

void window_capture_cursor(bool capture)
{
    ASSERT(ms_window);
    if (capture != ms_cursorCaptured)
    {
        ms_cursorCaptured = capture;
        glfwSetInputMode(
            ms_window,
            GLFW_CURSOR,
            capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

GLFWwindow* window_ptr(void)
{
    ASSERT(ms_window);
    return ms_window;
}

// ----------------------------------------------------------------------------

static GLFWwindow* CreateGlfwWindow(
    i32 width,
    i32 height,
    const char* title,
    bool fullscreen)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);

    i32 xpos, ypos;
    if (fullscreen)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        ASSERT(monitor);
        glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);
    }

    GLFWwindow* window = glfwCreateWindow(
        width,
        height,
        title,
        NULL,
        NULL);

    if (fullscreen)
    {
        glfwSetWindowPos(window, xpos, ypos);
    }

    return window;
}

static void OnGlfwError(i32 error_code, const char* description)
{
    fd_puts(description, fd_stderr);
    ASSERT(false);
}
