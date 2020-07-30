#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cmdstat_ok = 0,     // command completed
    cmdstat_err,        // parse error

    cmdstat_COUNT
} cmdstat_t;

typedef cmdstat_t(*cmdfn_t)(i32 argc, const char** argv);

void cmd_sys_init(void);
void cmd_sys_update(void);
void cmd_sys_shutdown(void);

void cmd_reg(const char* name, cmdfn_t fn);
bool cmd_exists(const char* name);
const char* cmd_complete(const char* namePart);
cmdstat_t cmd_exec(const char* line);
void cmd_text(const char* text);

PIM_C_END
