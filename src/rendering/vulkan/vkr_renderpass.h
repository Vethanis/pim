#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkRenderPass vkrRenderPass_Get(const vkrRenderPassDesc* desc);
void vkrRenderPass_Clear(void);

PIM_C_END
