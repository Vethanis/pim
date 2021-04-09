#pragma once

#include "rendering/vulkan/vkr.h"
#include "rendering/framebuffer.h"

PIM_C_BEGIN

bool vkrScreenBlit_New(VkRenderPass renderPass);
void vkrScreenBlit_Del(void);

void vkrScreenBlit_Blit(
    const vkrPassContext* passCtx,
    const FrameBuf* fbuf);

PIM_C_END
