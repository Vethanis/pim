#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct midihdl_s
{
    u32 version : 8;
    u32 index : 24;
} midihdl_t;

typedef struct midimsg_s
{
    u32 tick;
    u8 command;
    u8 param1;
    u8 param2;
} midimsg_t;

typedef void(*midi_cb)(const midimsg_t* msg, void* usr);

void midi_sys_init(void);
void midi_sys_update(void);
void midi_sys_shutdown(void);

i32 midi_devcount(void);
bool midi_exists(midihdl_t hdl);
midihdl_t midi_open(i32 port, midi_cb cb, void* usr);
bool midi_close(midihdl_t hdl);

PIM_C_END
