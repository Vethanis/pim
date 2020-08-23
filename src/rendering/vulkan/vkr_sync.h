#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkSemaphore vkrCreateSemaphore(void);
void vkrDestroySemaphore(VkSemaphore sema);

VkFence vkrCreateFence(bool signalled);
void vkrDestroyFence(VkFence fence);
void vkrResetFence(VkFence fence);
void vkrWaitFence(VkFence fence);

PIM_C_END
