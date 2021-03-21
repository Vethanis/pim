#include "rendering/vulkan/vkr_display.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "rendering/r_window.h"
#include "input/input_system.h"
#include <string.h>
#include <GLFW/glfw3.h>

static bool EnsureGlfw(void)
{
    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);
    return rv == GLFW_TRUE;
}

bool vkrDisplay_MonitorSize(i32* widthOut, i32* heightOut)
{
    *widthOut = 0;
    *heightOut = 0;
    if (!EnsureGlfw())
    {
        return false;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    ASSERT(monitor);
    if (!monitor)
    {
        return false;
    }

    i32 xpos = 0;
    i32 ypos = 0;
    i32 width = 0;
    i32 height = 0;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);
    if ((width > 0) && (height > 0))
    {
        *widthOut = width;
        *heightOut = height;
        return true;
    }

    return false;
}

bool vkrDisplay_New(vkrDisplay* display, i32 width, i32 height, const char* title)
{
    ASSERT(display);
    memset(display, 0, sizeof(*display));

    if (!EnsureGlfw())
    {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    display->window = window;
    ASSERT(window);
    if (!window)
    {
        goto cleanup;
    }
    Input_RegWindow(window);

    glfwGetFramebufferSize(window, &display->width, &display->height);

    VkSurfaceKHR surface = NULL;
    VkCheck(glfwCreateWindowSurface(g_vkr.inst, window, NULL, &surface));
    display->surface = surface;
    ASSERT(surface);
    if (!surface)
    {
        goto cleanup;
    }
    return true;
cleanup:
    vkrDisplay_Del(display);
    return false;
}

void vkrDisplay_Del(vkrDisplay* display)
{
    if (display)
    {
        if (display->surface)
        {
            vkDestroySurfaceKHR(g_vkr.inst, display->surface, NULL);
        }
        if (display->window)
        {
            glfwDestroyWindow(display->window);
        }
        memset(display, 0, sizeof(*display));
    }
}

ProfileMark(pm_updatesize, vkrDisplay_UpdateSize)
bool vkrDisplay_UpdateSize(vkrDisplay* display)
{
    ProfileBegin(pm_updatesize);

    ASSERT(display);
    ASSERT(display->window);
    i32 prevWidth = display->width;
    i32 prevHeight = display->height;
    glfwGetFramebufferSize(display->window, &display->width, &display->height);
    bool changed = (prevWidth != display->width) || (prevHeight != display->height);

    ProfileEnd(pm_updatesize);
    return changed;
}

ProfileMark(pm_isopen, vkrDisplay_IsOpen)
bool vkrDisplay_IsOpen(const vkrDisplay* display)
{
    ProfileBegin(pm_isopen);

    ASSERT(display);
    ASSERT(display->window);
    GLFWwindow* window = display->window;
    bool isopen = !glfwWindowShouldClose(window);

    ProfileEnd(pm_isopen);
    return isopen;
}

ProfileMark(pm_pollevents, vkrDisplay_PollEvents)
void vkrDisplay_PollEvents(const vkrDisplay* display)
{
    ProfileBegin(pm_pollevents);

    ASSERT(display);
    ASSERT(display->window);
    glfwPollEvents();

    ProfileEnd(pm_pollevents);
}
