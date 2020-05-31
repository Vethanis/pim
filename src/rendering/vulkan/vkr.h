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

typedef struct vkrDevice
{
    VkInstance inst;
    VkPhysicalDevice phdev;
    VkDevice dev;
    VkQueue gfxq;
    VkQueue xferq;
    VkQueue compq;
} vkrDevice;

extern GLFWwindow* g_vkrwindow;
extern vkrDevice g_vkr;

const VkPhysicalDeviceFeatures* vkrPhDevFeats(void);
const VkPhysicalDeviceProperties* vkrPhDevProps(void);
const VkPhysicalDeviceLimits* vkrPhDevLimits(void);

void vkr_init(void);
void vkr_update(void);
void vkr_shutdown(void);

PIM_C_END
