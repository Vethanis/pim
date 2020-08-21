#include "rendering/vulkan/vkr_display.h"
#include "allocator/allocator.h"
#include <string.h>
#include <GLFW/glfw3.h>

vkrDisplay* vkrDisplay_New(i32 width, i32 height, const char* title)
{
    vkrDisplay* display = perm_calloc(sizeof(*display));
    display->refcount = 1;

    i32 rv = glfwInit();
    ASSERT(rv == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

bool vkrDisplay_UpdateSize(vkrDisplay* display)
{
    ASSERT(vkrAlive(display));
    ASSERT(display->window);
    i32 prevWidth = display->width;
    i32 prevHeight = display->height;
    glfwGetWindowSize(display->window, &display->width, &display->height);
    return (prevWidth != display->width) || (prevHeight != display->height);
}
