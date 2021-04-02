#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct FrameBuf_s FrameBuf;

bool RenderSys_Init(void);
void RenderSys_Update(void);
void RenderSys_Shutdown(void);
void RenderSys_Gui(bool* pEnabled);

FrameBuf* RenderSys_FrontBuf(void);
FrameBuf* RenderSys_BackBuf(void);

PIM_C_END
