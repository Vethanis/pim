#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrOpaquePass_New(vkrOpaquePass* pass, VkRenderPass renderPass);
void vkrOpaquePass_Del(vkrOpaquePass* pass);

void vkrOpaquePass_Draw(const vkrPassContext* passCtx, vkrOpaquePass* pass);

PIM_C_END
