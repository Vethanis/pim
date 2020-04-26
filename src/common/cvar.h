#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    cvar_text = 0,
    cvar_float,
    cvar_int,
    cvar_bool,
} cvar_type;

typedef struct cvar_s
{
    cvar_type type;
    const char* name;
    char value[32];
    const char* description;
    float asFloat;
} cvar_t;

void cvar_reg(cvar_t* ptr);
cvar_t* cvar_find(const char* name);
const char* cvar_complete(const char* namePart);
void cvar_set_str(cvar_t* ptr, const char* value);
void cvar_set_float(cvar_t* ptr, float value);

void cvar_gui(bool* pEnabled);

PIM_C_END
