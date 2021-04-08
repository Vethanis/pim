#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrExposurePass_New(void);
void vkrExposurePass_Del(void);

void vkrExposurePass_Setup(void);
void vkrExposurePass_Execute(void);

vkrExposure* vkrExposurePass_GetParams(void);
void vkrExposurePass_SetParams(const vkrExposure* params);
float vkrExposurePass_GetExposure(void);
const vkrBuffer* vkrExposurePass_GetExposureBuffer(void);

PIM_C_END
