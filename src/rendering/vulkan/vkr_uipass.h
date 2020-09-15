#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrUIPass_New(vkrUIPass* pass, VkRenderPass renderPass);
void vkrUIPass_Del(vkrUIPass* pass);

void vkrUIPass_Draw(const vkrPassContext* passCtx, vkrUIPass* pass);

PIM_C_END
