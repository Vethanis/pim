#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrDisplay_MonitorSize(i32* widthOut, i32* heightOut);

bool vkrDisplay_New(vkrDisplay* display, i32 width, i32 height, const char* title);
void vkrDisplay_Del(vkrDisplay* display);

// returns true when width or height has changed
bool vkrDisplay_UpdateSize(vkrDisplay* display);

// returns false when the display window should close
bool vkrDisplay_IsOpen(const vkrDisplay* display);

// processes windowing and input event queue
void vkrDisplay_PollEvents(const vkrDisplay* display);

PIM_C_END
