#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrOpaquePass_New(void);
void vkrOpaquePass_Del(void);

void vkrOpaquePass_Setup(void);
void vkrOpaquePass_Execute(vkrPassContext const *const passCtx);

PIM_C_END
