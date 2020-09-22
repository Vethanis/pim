#pragma once

#include "common/macro.h"

PIM_C_BEGIN

i32 r_width_get(void);
void r_width_set(i32 width);

i32 r_height_get(void);
void r_height_set(i32 height);

float r_aspect_get(void);

float r_scale_get(void);
void r_scale_set(float scale);

i32 r_scaledwidth_get(void);
i32 r_scaledheight_get(void);

PIM_C_END
