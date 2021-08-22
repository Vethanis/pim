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

void cmd_reg(const char* name, const char* argDocs, const char* helpText, cmd_fn_t fn);
bool cmd_exists(const char* name);
const char* cmd_complete(const char* namePart);

// pushes to back of command queue, then executes queue.
cmdstat_t cmd_enqueue(const char* text);

// pushes to front of command queue in reverse order, then executes queue.
cmdstat_t cmd_immediate(const char* text);

const char* cmd_parse(const char* text, char* tokenOut, i32 bufSize);

// parse an option for your command.
// if user passes "cmd -key=value" or "cmd -key value", returns pointer to value.
// if user passes "cmd -key" or "cmd -key -bar", returns pointer to empty string.
// if user passes "cmd", returns NULL.
const char* cmd_getopt(i32 argc, const char** argv, const char* key);

PIM_C_END
