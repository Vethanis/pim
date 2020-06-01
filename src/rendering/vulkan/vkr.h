#pragma once

#include "common/macro.h"

PIM_C_BEGIN

PIM_FWD_DECL(GLFWwindow);
PIM_FWD_DECL(VkPhysicalDeviceFeatures);
PIM_FWD_DECL(VkPhysicalDeviceProperties);
PIM_FWD_DECL(VkPhysicalDeviceLimits);
PIM_DECL_HANDLE(VkInstance);
PIM_DECL_HANDLE(VkPhysicalDevice);
PIM_DECL_HANDLE(VkDevice);
PIM_DECL_HANDLE(VkQueue);
PIM_DECL_HANDLE(VkSurfaceKHR);
PIM_DECL_HANDLE(VkSwapchainKHR);
PIM_DECL_HANDLE(VkImage);

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
    GLFWwindow* win;
    VkSurfaceKHR surf;
    VkSwapchainKHR swap;
    u32 imgCount;
    VkImage* images;
} vkr_t;

extern vkr_t g_vkr;

const VkPhysicalDeviceFeatures* vkrPhDevFeats(void);
const VkPhysicalDeviceProperties* vkrPhDevProps(void);
const VkPhysicalDeviceLimits* vkrPhDevLimits(void);

void vkr_init(void);
void vkr_update(void);
void vkr_shutdown(void);

PIM_C_END
