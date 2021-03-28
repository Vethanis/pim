#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cmdstat_ok,
    cmdstat_err,

    cmdstat_COUNT
} cmdstat_t;

typedef cmdstat_t(*cmd_fn_t)(i32 argc, const char** argv);

void cmd_sys_init(void);
void cmd_sys_update(void);
void cmd_sys_shutdown(void);

void cmd_reg(const char* name, cmd_fn_t fn);
bool cmd_exists(const char* name);
const char* cmd_complete(const char* namePart);
cmdstat_t cmd_text(const char* text);
const char* cmd_parse(const char* text, char* tokenOut, i32 bufSize);

PIM_C_END
