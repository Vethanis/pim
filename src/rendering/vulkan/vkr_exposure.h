#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrExposure* vkrExposure_GetParams(void);
float vkrGetAvgLuminance(void);
float vkrGetExposure(void);
float vkrGetMaxLuminance(void);
float vkrGetMinLuminance(void);

bool vkrExposure_Init(void);
void vkrExposure_Shutdown(void);

void vkrExposure_Setup(void);
void vkrExposure_Execute(void);

PIM_C_END
