#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrDisplay* vkrDisplay_New(i32 width, i32 height, const char* title);
void vkrDisplay_Retain(vkrDisplay* display);
void vkrDisplay_Release(vkrDisplay* display);

// returns true when width or height has changed
bool vkrDisplay_UpdateSize(vkrDisplay* display);

PIM_C_END
