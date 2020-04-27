#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cvart_text = 0,
    cvart_float,
    cvart_int,
    cvart_bool,
} cvar_type;

typedef enum
{
    cvarf_dirty = 1 << 0,   // modified in the last frame
    cvarf_save = 1 << 1,    // save value to config file

} cvar_flags;

typedef struct cvar_s
{
    cvar_type type;
    u32 flags;
    const char* name;
    char value[32];
    const char* description;
    float asFloat;
} cvar_t;

// registers your cvar to the cvar system
void cvar_reg(cvar_t* ptr);

// attempts to find a cvar with matching name
cvar_t* cvar_find(const char* name);

// attempts to auto-complete namePart to a cvar name
const char* cvar_complete(const char* namePart);

// updates string and float value, sets dirty flag
void cvar_set_str(cvar_t* ptr, const char* value);

// updates string and float value, sets dirty flag
void cvar_set_float(cvar_t* ptr, float value);

// checks and clears dirty flag
bool cvar_check_dirty(cvar_t* ptr);

// displays the cvar gui
void cvar_gui(bool* pEnabled);

PIM_C_END
