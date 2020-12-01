#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrOpaquePass_New(vkrOpaquePass *const pass, VkRenderPass renderPass);
void vkrOpaquePass_Del(vkrOpaquePass *const pass);

void vkrOpaquePass_Setup(vkrOpaquePass *const pass);
void vkrOpaquePass_Execute(
    vkrPassContext const *const passCtx,
    vkrOpaquePass *const pass);

PIM_C_END
