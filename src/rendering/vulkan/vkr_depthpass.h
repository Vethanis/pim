#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDepthPass_New(vkrDepthPass *const pass, VkRenderPass renderPass);
void vkrDepthPass_Del(vkrDepthPass *const pass);

void vkrDepthPass_Setup(vkrDepthPass *const pass);
void vkrDepthPass_Execute(
    vkrPassContext const *const passCtx,
    vkrDepthPass *const pass);

PIM_C_END
