#pragma once

#include "common/macro.h"
#include <volk/volk.h>
#include "containers/strlist.h"

PIM_C_BEGIN

PIM_FWD_DECL(GLFWwindow);

#define VkCheck(expr) do { VkResult _res = (expr); ASSERT(_res == VK_SUCCESS); } while(0)

typedef enum
{
    vkrQueueId_Pres,
    vkrQueueId_Gfx,
    vkrQueueId_Comp,
    vkrQueueId_Xfer,

    vkrQueueId_COUNT
} vkrQueueId;

typedef struct vkr_t
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    VkQueue queues[vkrQueueId_COUNT];
    VkQueueFamilyProperties queueProps[vkrQueueId_COUNT];
    VkPhysicalDeviceFeatures phdevFeats;
    VkPhysicalDeviceProperties phdevProps;
    VkDebugUtilsMessengerEXT messenger;
} vkr_t;
extern vkr_t g_vkr;

typedef struct vkrDisplay
{
    GLFWwindow* win;
    VkSurfaceKHR surf;
    VkSwapchainKHR swap;
    u32 imgCount;
    VkImage* images;
    VkImageView* views;
    i32 format;
    i32 colorSpace;
    i32 width;
    i32 height;
} vkrDisplay;
extern vkrDisplay g_vkrdisp;

void vkr_init(i32 width, i32 height);
void vkr_update(void);
void vkr_shutdown(void);

PIM_C_END
