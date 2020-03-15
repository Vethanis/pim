#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct cvar_s
{
    char name[30];
    char value[30];
    float asFloat;
} cvar_t;

void cvar_create(cvar_t* ptr, const char* name, const char* value);
cvar_t* cvar_find(const char* name);
void cvar_set_str(cvar_t* ptr, const char* value);
void cvar_set_float(cvar_t* ptr, float value);

PIM_C_END
