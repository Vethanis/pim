#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDepthPass_New(vkrDepthPass* pass, VkRenderPass renderPass);
void vkrDepthPass_Del(vkrDepthPass* pass);

void vkrDepthPass_Draw(const vkrPassContext* passCtx, vkrDepthPass* pass);

PIM_C_END
