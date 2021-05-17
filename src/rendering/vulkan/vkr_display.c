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

bool vkrDisplay_GetWorkSize(i32* widthOut, i32* heightOut)
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

bool vkrDisplay_GetFullSize(i32* widthOut, i32* heightOut)
{
    *widthOut = 0;
    *heightOut = 0;
    if (!EnsureGlfw())
    {
        return false;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor)
    {
        ASSERT(false);
        return false;
    }

    i32 modeCount = 0;
    const GLFWvidmode* modes = glfwGetVideoModes(monitor, &modeCount);
    if (!modes || (modeCount <= 0))
    {
        ASSERT(false);
        return false;
    }

    i32 chosen = -1;
    i32 chosenArea = 0;
    for (i32 i = 0; i < modeCount; ++i)
    {
        i32 area = modes[i].width * modes[i].height;
        if (area > chosenArea)
        {
            chosen = i;
            chosenArea = area;
        }
    }

    if (chosen >= 0)
    {
        *widthOut = modes[chosen].width;
        *heightOut = modes[chosen].height;
        return true;
    }

    ASSERT(false);
    return false;
}

bool vkrDisplay_GetSize(i32* widthOut, i32* heightOut, bool fullscreen)
{
    if (fullscreen)
    {
        return vkrDisplay_GetFullSize(widthOut, heightOut);
    }
    else
    {
        return vkrDisplay_GetWorkSize(widthOut, heightOut);
    }
}

bool vkrWindow_New(
    vkrWindow* vkwin,
    const char* title,
    i32 width, i32 height,
    bool fullscreen)
{
    ASSERT(vkwin);
    memset(vkwin, 0, sizeof(*vkwin));
    vkwin->fullscreen = fullscreen;

    if (!EnsureGlfw())
    {
        return false;
    }

    GLFWwindow* handle = NULL;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    if (!fullscreen)
    {
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        handle = glfwCreateWindow(width, height, title, NULL, NULL);
    }
    else
    {
        handle = glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), NULL);
    }
    vkwin->handle = handle;
    if (!handle)
    {
        ASSERT(false);
        goto cleanup;
    }
    Input_RegWindow(handle);

    glfwGetFramebufferSize(handle, &vkwin->width, &vkwin->height);

    VkSurfaceKHR surface = NULL;
    VkCheck(glfwCreateWindowSurface(g_vkr.inst, handle, NULL, &surface));
    vkwin->surface = surface;
    ASSERT(surface);
    if (!surface)
    {
        goto cleanup;
    }
    return true;
cleanup:
    vkrWindow_Del(vkwin);
    return false;
}

void vkrWindow_Del(vkrWindow* vkwin)
{
    if (vkwin)
    {
        if (vkwin->surface)
        {
            vkDestroySurfaceKHR(g_vkr.inst, vkwin->surface, NULL);
        }
        if (vkwin->handle)
        {
            glfwDestroyWindow(vkwin->handle);
        }
        memset(vkwin, 0, sizeof(*vkwin));
    }
}

ProfileMark(pm_updatesize, vkrWindow_UpdateSize)
bool vkrWindow_UpdateSize(vkrWindow* window)
{
    ProfileBegin(pm_updatesize);

    ASSERT(window);
    ASSERT(window->handle);
    i32 prevWidth = window->width;
    i32 prevHeight = window->height;
    glfwGetFramebufferSize(window->handle, &window->width, &window->height);
    bool changed = (prevWidth != window->width) || (prevHeight != window->height);

    ProfileEnd(pm_updatesize);
    return changed;
}

ProfileMark(pm_getposition, vkrWindow_GetPosition)
void vkrWindow_GetPosition(vkrWindow* window, i32* xOut, i32* yOut)
{
    ProfileBegin(pm_getposition);

    glfwGetWindowPos(window->handle, xOut, yOut);

    ProfileEnd(pm_getposition);
}

ProfileMark(pm_setposition, vkrWindow_SetPosition)
void vkrWindow_SetPosition(vkrWindow* window, i32 x, i32 y)
{
    ProfileBegin(pm_setposition);

    glfwSetWindowPos(window->handle, x, y);

    ProfileEnd(pm_setposition);
}

ProfileMark(pm_isopen, vkrWindow_IsOpen)
bool vkrWindow_IsOpen(const vkrWindow* window)
{
    ProfileBegin(pm_isopen);

    ASSERT(window);
    ASSERT(window->handle);
    bool isopen = !glfwWindowShouldClose(window->handle);

    ProfileEnd(pm_isopen);
    return isopen;
}

ProfileMark(pm_pollevents, vkrWindow_Poll)
void vkrWindow_Poll(void)
{
    ProfileBegin(pm_pollevents);

    glfwPollEvents();

    ProfileEnd(pm_pollevents);
}
