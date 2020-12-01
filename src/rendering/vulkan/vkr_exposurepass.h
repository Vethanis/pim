#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrExposurePass_New(vkrExposurePass *const pass);
void vkrExposurePass_Del(vkrExposurePass *const pass);

void vkrExposurePass_Setup(vkrExposurePass *const pass);
void vkrExposurePass_Execute(vkrExposurePass *const pass);

PIM_C_END
