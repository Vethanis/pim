#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrExposure_Init(void);
void vkrExposure_Shutdown(void);

void vkrExposure_Setup(void);
void vkrExposure_Execute(void);

vkrExposure* vkrExposure_GetParams(void);

PIM_C_END
