#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrPass_New(vkrPass *const pass, vkrPassDesc const *const desc);
void vkrPass_Del(vkrPass *const pass);

PIM_C_END
