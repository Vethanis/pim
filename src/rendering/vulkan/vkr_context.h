#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrFrameContext* vkrContext_Get(void);

bool vkrContext_New(vkrContext* ctx);
void vkrContext_Del(vkrContext* ctx);

bool vkrThreadContext_New(vkrThreadContext* ctx);
void vkrThreadContext_Del(vkrThreadContext* ctx);

bool vkrFrameContext_New(vkrFrameContext* ctx);
void vkrFrameContext_Del(vkrFrameContext* ctx);

void vkrContext_WritePerDraw(
    vkrFrameContext* ctx,
    const vkrPerDraw* perDraws,
    i32 length);

void vkrContext_WritePerCamera(
    vkrFrameContext* ctx,
    const vkrPerCamera* perCameras,
    i32 length);

PIM_C_END
