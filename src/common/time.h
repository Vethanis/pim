#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void TimeSys_Init(void);
void TimeSys_Update(void);
void TimeSys_Shutdown(void);

u32 Time_FrameCount(void);
u64 Time_AppStart(void);
u64 Time_FrameStart(void);

u64 Time_Now(void);
u64 Time_Delta(void);
double Time_Deltaf(void);

double Time_Sec(u64 ticks);
double Time_Milli(u64 ticks);
double Time_Micro(u64 ticks);

PIM_C_END
