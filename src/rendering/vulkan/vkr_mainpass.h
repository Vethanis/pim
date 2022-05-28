#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrMainPass_New(void);
void vkrMainPass_Del(void);

void vkrMainPass_Setup(void);
void vkrMainPass_Execute(void);

PIM_C_END
