#pragma once

#include "rendering/vulkan/vkr.h"
#include "containers/strlist.h"

PIM_C_BEGIN

bool vkrInstance_Init(vkrSys* vkr);
void vkrInstance_Shutdown(vkrSys* vkr);

// ----------------------------------------------------------------------------

VkInstance vkrCreateInstance(StrList extensions, StrList layers);

PIM_C_END
