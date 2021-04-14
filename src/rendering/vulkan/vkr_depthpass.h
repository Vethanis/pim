#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDepthPass_New(void);
void vkrDepthPass_Del(void);

void vkrDepthPass_Setup(void);
void vkrDepthPass_Execute(vkrPassContext const *const passCtx);

PIM_C_END
