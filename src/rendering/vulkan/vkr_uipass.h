#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrUIPass_New(vkrUIPass *const pass, VkRenderPass renderPass);
void vkrUIPass_Del(vkrUIPass *const pass);

void vkrUIPass_Setup(vkrUIPass *const pass);
void vkrUIPass_Execute(
    vkrPassContext const *const passCtx,
    vkrUIPass *const pass);

PIM_C_END
