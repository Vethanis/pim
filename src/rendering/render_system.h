#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct FrameBuf_s FrameBuf;

bool RenderSys_Init(void);
bool RenderSys_WindowUpdate(void);
void RenderSys_Update(void);
void RenderSys_Shutdown(void);
void RenderSys_Gui(bool* pEnabled);

FrameBuf* RenderSys_FrontBuf(void);

PIM_C_END
