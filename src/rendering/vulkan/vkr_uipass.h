#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrUIPass_New(void);
void vkrUIPass_Del(void);

void vkrUIPass_Setup(void);
void vkrUIPass_Execute(void);

PIM_C_END
