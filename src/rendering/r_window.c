#include "rendering/r_window.h"
#include <GLFW/glfw3.h>
#include "common/time.h"
#include "common/cvar.h"
#include "threading/intrin.h"
#include "threading/sleep.h"
#include "common/profiler.h"
#include "common/console.h"
#include "input/input_system.h"
#include "rendering/vulkan/vkr.h"
#include <math.h>

static cvar_t cv_fpslimit =
{
    .type = cvart_int,
    .name = "fps_limit",
    .value = "1000",
    .minInt = 1,
    .maxInt = 1000,
    .desc = "limits fps when above this value"
};

static i32 ms_width;
static i32 ms_height;
static i32 ms_target;
static u64 ms_lastSwap;

static void OnGlfwError(i32 error_code, const char* description);

// ----------------------------------------------------------------------------

void window_sys_init(void)
{
    cvar_reg(&cv_fpslimit);
    ms_lastSwap = time_now();

    glfwSetErrorCallback(OnGlfwError);
    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);
}

void window_sys_update(void)
{
}

void window_sys_shutdown(void)
{
    glfwTerminate();
}

i32 window_width(void)
{
    return g_vkr.display.height;
}

i32 window_height(void)
{
    return g_vkr.display.width;
}

bool window_is_open(void)
{
    if (g_vkr.display.window)
    {
        return !glfwWindowShouldClose(g_vkr.display.window);
    }
    return false;
}

void window_close(bool shouldClose)
{
    glfwSetWindowShouldClose(g_vkr.display.window, shouldClose);
}

i32 window_get_target(void)
{
    return cv_fpslimit.asInt;
}

void window_set_target(i32 fps)
{
    cvar_set_int(&cv_fpslimit, fps);
}

ProfileMark(pm_waitfps, wait_for_target_fps)
static void wait_for_target_fps(void)
{
    ProfileBegin(pm_waitfps);

    const double targetMS = 1000.0 / cv_fpslimit.asInt;
    const double diffMS = targetMS - time_milli(time_now() - ms_lastSwap);
    if (diffMS >= 1.0)
    {
        intrin_sleep((u32)diffMS);
    }

    ms_lastSwap = time_now();

    ProfileEnd(pm_waitfps);
}

void window_swapbuffers(void)
{
    wait_for_target_fps();
}

GLFWwindow* window_get(void)
{
    return g_vkr.display.window;
}

static void OnGlfwError(i32 error_code, const char* description)
{
    con_logf(LogSev_Error, "glfw", "%s", description);
    ASSERT(false);
}
