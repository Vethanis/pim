#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrPass_New(vkrPass *const pass, vkrPassDesc const *const desc);
void vkrPass_Del(vkrPass *const pass);

void vkrCmdBindPass(VkCommandBuffer cmd, const vkrPass* pass);
void vkrCmdPushConstants(VkCommandBuffer cmd, const vkrPass* pass, const void* src, i32 bytes);

PIM_C_END
