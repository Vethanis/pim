#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrImGuiPass_New(vkrImGui* imgui, VkRenderPass renderPass);
void vkrImGuiPass_Del(vkrImGui* imgui);

void vkrImGuiPass_Draw(
    vkrImGui* imgui,
    VkCommandBuffer cmd);

PIM_C_END
