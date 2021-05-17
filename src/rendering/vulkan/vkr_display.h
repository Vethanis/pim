#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

// usable windowed size
bool vkrDisplay_GetWorkSize(i32* widthOut, i32* heightOut);
// usable fullscreen size
bool vkrDisplay_GetFullSize(i32* widthOut, i32* heightOut);
bool vkrDisplay_GetSize(i32* widthOut, i32* heightOut, bool fullscreen);

bool vkrWindow_New(
    vkrWindow* window,
    const char* title,
    i32 width, i32 height,
    bool fullscreen);
void vkrWindow_Del(vkrWindow* window);

// returns true when width or height has changed
bool vkrWindow_UpdateSize(vkrWindow* window);

void vkrWindow_GetPosition(vkrWindow* window, i32* xOut, i32* yOut);
void vkrWindow_SetPosition(vkrWindow* window, i32 x, i32 y);

// returns false when the display window should close
bool vkrWindow_IsOpen(const vkrWindow* window);

// processes windowing and input event queue
void vkrWindow_Poll(void);

PIM_C_END
