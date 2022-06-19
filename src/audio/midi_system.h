#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct MidiHdl_s
{
    u32 version : 8;
    u32 index : 24;
} MidiHdl;

typedef struct MidiMsg_s
{
    i32 port;
    u8 command;
    u8 param1;
    u8 param2;
} MidiMsg;

typedef void(*OnMidiFn)(const MidiMsg* msg, void* usr);

void MidiSys_Init(void);
void MidiSys_Update(void);
void MidiSys_Shutdown(void);

i32 Midi_DeviceCount(void);
bool Midi_Exists(MidiHdl hdl);
MidiHdl Midi_Open(i32 port, OnMidiFn cb, void* usr);
bool Midi_Close(MidiHdl hdl);

PIM_C_END
