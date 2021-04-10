#include "rendering/r_window.h"
#include <GLFW/glfw3.h>
#include "common/time.h"
#include "common/cvars.h"
#include "threading/intrin.h"
#include "threading/sleep.h"
#include "common/profiler.h"
#include "common/console.h"
#include "input/input_system.h"
#include "rendering/vulkan/vkr.h"
#include <math.h>

static i32 ms_width;
static i32 ms_height;
static i32 ms_target;
static u64 ms_lastSwap;

static void OnGlfwError(i32 error_code, const char* description);

// ----------------------------------------------------------------------------

void WinSys_Init(void)
{
    ms_lastSwap = Time_Now();

    glfwSetErrorCallback(OnGlfwError);
    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);
}

void WinSys_Update(void)
{
}

void WinSys_Shutdown(void)
{
    glfwTerminate();
}

i32 Window_Width(void)
{
    return g_vkr.display.height;
}

i32 Window_Height(void)
{
    return g_vkr.display.width;
}

bool Window_IsOpen(void)
{
    if (g_vkr.display.window)
    {
        return !glfwWindowShouldClose(g_vkr.display.window);
    }
    return false;
}

void Window_Close(bool shouldClose)
{
    glfwSetWindowShouldClose(g_vkr.display.window, shouldClose);
}

i32 window_get_target(void)
{
    return ConVar_GetInt(&cv_r_fpslimit);
}

void window_set_target(i32 fps)
{
    ConVar_SetInt(&cv_r_fpslimit, fps);
}

ProfileMark(pm_waitfps, WaitForTargetFps)
static void WaitForTargetFps(void)
{
    ProfileBegin(pm_waitfps);

    const double targetMS = 1000.0 / window_get_target();
    const double diffMS = targetMS - Time_Milli(Time_Now() - ms_lastSwap);
    if (diffMS > 1.5)
    {
        Intrin_Sleep((u32)(diffMS - 0.5));
    }

    ms_lastSwap = Time_Now();

    ProfileEnd(pm_waitfps);
}

void Window_SwapBuffers(void)
{
    WaitForTargetFps();
}

GLFWwindow* Window_Get(void)
{
    return g_vkr.display.window;
}

static void OnGlfwError(i32 error_code, const char* description)
{
    Con_Logf(LogSev_Error, "glfw", "%s", description);
    ASSERT(false);
}
