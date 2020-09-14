#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

void ExposeImage(int2 size, float4* pim_noalias light, vkrExposure* parameters);

PIM_C_END
