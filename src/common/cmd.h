#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cmdsrc_client = 0,
    cmdsrc_buffer,

    cmdsrc_COUNT
} cmdsrc_t;

typedef enum
{
    cmdstat_ok = 0,     // command completed
    cmdstat_err,        // parse error
    cmdstat_yield,      // yield execution until next frame

    cmdstat_COUNT
} cmdstat_t;

typedef cmdstat_t(*cmdfn_t)(i32 argc, const char** argv);

typedef struct cbuf_s
{
    char* ptr;
    i32 length;
    EAlloc allocator;
} cbuf_t;

void cmd_sys_init(void);
void cmd_sys_update(void);
void cmd_sys_shutdown(void);

void cbuf_new(cbuf_t* buf, EAlloc allocator);
void cbuf_del(cbuf_t* buf);
void cbuf_clear(cbuf_t* buf);
void cbuf_reserve(cbuf_t* buf, i32 size);
void cbuf_pushfront(cbuf_t* buf, const char* text);
void cbuf_pushback(cbuf_t* buf, const char* text);
cmdstat_t cbuf_exec(cbuf_t* buf);

void cmd_reg(const char* name, cmdfn_t fn);
bool cmd_exists(const char* name);
const char* cmd_complete(const char* namePart);
cmdstat_t cmd_exec(cbuf_t* buf, const char* line, cmdsrc_t src);

PIM_C_END
