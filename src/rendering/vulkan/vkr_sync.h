#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkSemaphore vkrSemaphore_New(void);
void vkrSemaphore_Del(VkSemaphore sema);

VkFence vkrFence_New(bool signalled);
void vkrFence_Del(VkFence fence);
void vkrFence_Reset(VkFence fence);
void vkrFence_Wait(VkFence fence);
vkrFenceState vkrFence_Stat(VkFence fence);

PIM_C_END
