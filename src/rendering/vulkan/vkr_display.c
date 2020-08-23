#include "rendering/vulkan/vkr_display.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>
#include <GLFW/glfw3.h>

vkrDisplay* vkrDisplay_New(i32 width, i32 height, const char* title)
{
    vkrDisplay* display = perm_calloc(sizeof(*display));
    display->refcount = 1;

    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    ASSERT(window);
    display->window = window;

    glfwGetWindowSize(window, &display->width, &display->height);

    VkSurfaceKHR surface = NULL;
    VkCheck(glfwCreateWindowSurface(g_vkr.inst, window, NULL, &surface));
    ASSERT(surface);
    display->surface = surface;

    return display;
}

void vkrDisplay_Retain(vkrDisplay* display)
{
    if (vkrAlive(display))
    {
        display->refcount += 1;
    }
}

void vkrDisplay_Release(vkrDisplay* display)
{
    if (vkrAlive(display))
    {
        display->refcount -= 1;
        if (display->refcount == 0)
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
            pim_free(display);
        }
    }
}

ProfileMark(pm_updatesize, vkrDisplay_UpdateSize)
bool vkrDisplay_UpdateSize(vkrDisplay* display)
{
    ProfileBegin(pm_updatesize);

    ASSERT(vkrAlive(display));
    ASSERT(display->window);
    i32 prevWidth = display->width;
    i32 prevHeight = display->height;
    glfwGetWindowSize(display->window, &display->width, &display->height);
    bool changed = (prevWidth != display->width) || (prevHeight != display->height);

    ProfileEnd(pm_updatesize);
    return changed;
}

ProfileMark(pm_isopen, vkrDisplay_IsOpen)
bool vkrDisplay_IsOpen(const vkrDisplay* display)
{
    ProfileBegin(pm_isopen);

    ASSERT(vkrAlive(display));
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

    ASSERT(vkrAlive(display));
    ASSERT(display->window);
    glfwPollEvents();

    ProfileEnd(pm_pollevents);
}
