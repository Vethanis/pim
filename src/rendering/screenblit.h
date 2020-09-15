#pragma once

#include "rendering/vulkan/vkr.h"
#include "rendering/framebuffer.h"

PIM_C_BEGIN

bool vkrScreenBlit_New(vkrScreenBlit* blit, VkRenderPass renderPass);
void vkrScreenBlit_Del(vkrScreenBlit* blit);

void vkrScreenBlit_Blit(
    const vkrPassContext* passCtx,
    vkrScreenBlit* blit,
    const framebuf_t* fbuf);

PIM_C_END
