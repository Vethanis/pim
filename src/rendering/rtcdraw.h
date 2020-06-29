#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;
typedef struct camera_s camera_t;

void RtcDrawInit(void);
void RtcDrawShutdown(void);

void RtcDraw(framebuf_t* target, const camera_t* camera);

PIM_C_END
