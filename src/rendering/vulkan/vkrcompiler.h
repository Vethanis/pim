#pragma once

#include "common/macro.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrCompile(const vkrCompileInput* input, vkrCompileOutput* output);
void vkrCompileOutput_Del(vkrCompileOutput* output);

PIM_C_END
