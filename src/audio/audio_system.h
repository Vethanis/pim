#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void audio_sys_init(void);
void audio_sys_update(void);
void audio_sys_shutdown(void);
void audio_sys_ongui(bool* pEnabled);

u32 audio_sys_tick(void);

PIM_C_END
