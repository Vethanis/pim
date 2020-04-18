#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct cvar_s
{
    const char* name;
    char value[32];
    const char* description;
    float asFloat;
} cvar_t;

void cvar_reg(cvar_t* ptr);
void cvar_create(cvar_t* ptr, const char* name, const char* value, const char* desc);
cvar_t* cvar_find(const char* name);
void cvar_set_str(cvar_t* ptr, const char* value);
void cvar_set_float(cvar_t* ptr, float value);

PIM_C_END
