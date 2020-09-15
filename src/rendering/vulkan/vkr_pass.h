#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrPass_New(vkrPass* pass, const vkrPassDesc* desc);
void vkrPass_Del(vkrPass* pass);

PIM_C_END
