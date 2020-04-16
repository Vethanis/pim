#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void screenblit_init(i32 width, i32 height);
void screenblit_shutdown(void);

void screenblit_blit(const u32* texels, i32 width, i32 height);

PIM_C_END
