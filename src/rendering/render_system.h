#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;

void RenderSys_Init(void);
void RenderSys_Update(void);
void RenderSys_Shutdown(void);
void RenderSys_Gui(bool* pEnabled);

framebuf_t* RenderSys_FrontBuf(void);
framebuf_t* RenderSys_BackBuf(void);

PIM_C_END
