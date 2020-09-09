#pragma once

#include "common/macro.h"

PIM_DECL_HANDLE(VkCommandBuffer);
PIM_DECL_HANDLE(VkRenderPass);

PIM_C_BEGIN

bool screenblit_init(
    VkRenderPass renderPass,
    i32 subpass);
void screenblit_shutdown(void);

VkCommandBuffer screenblit_blit(
    const u32* texels,
    i32 width,
    i32 height,
    VkRenderPass renderPass);

PIM_C_END
