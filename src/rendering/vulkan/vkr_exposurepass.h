#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrExposurePass_New(vkrExposurePass* pass);
void vkrExposurePass_Del(vkrExposurePass* pass);

void vkrExposurePass_Execute(vkrExposurePass* pass);

PIM_C_END
