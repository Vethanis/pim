#pragma once

#include "common/macro.h"

PIM_DECL_HANDLE(VkCommandBuffer);
PIM_DECL_HANDLE(VkRenderPass);

PIM_C_BEGIN

bool screenblit_init(void);
void screenblit_shutdown(void);

void screenblit_blit(
    VkCommandBuffer cmd,
    const u32* texels,
    i32 width,
    i32 height);

PIM_C_END
