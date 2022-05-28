#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTargets_Init(void);
void vkrTargets_Shutdown(void);
void vkrTargets_Recreate(void);

PIM_C_END
