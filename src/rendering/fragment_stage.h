#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;

void drawables_fragment(framebuf_t* frontBuf, const framebuf_t* backBuf);

AmbCube_t VEC_CALL AmbCube_Get(void);
void VEC_CALL AmbCube_Set(AmbCube_t cube);

PIM_C_END
