#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cmdstat_ok,
    cmdstat_err,

    cmdstat_COUNT
} cmdstat_t;

typedef cmdstat_t(*CmdFn)(i32 argc, const char** argv);

void CmdSys_Init(void);
void CmdSys_Update(void);
void CmdSys_Shutdown(void);

void Cmd_Reg(const char* name, CmdFn fn);
bool Cmd_Exists(const char* name);
const char* Cmd_Complete(const char* namePart);
cmdstat_t Cmd_Text(const char* text);
const char* Cmd_Parse(const char* text, char* tokenOut, i32 bufSize);

PIM_C_END
