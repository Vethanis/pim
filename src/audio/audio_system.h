#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void AudioSys_Init(void);
void AudioSys_Update(void);
void AudioSys_Shutdown(void);
void AudioSys_Gui(bool* pEnabled);

PIM_C_END
