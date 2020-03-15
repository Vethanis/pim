#include "common/cvar.h"

#include "common/hashstring.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define kMaxCvars 256

static uint32_t ms_hashes[kMaxCvars];
static cvar_t* ms_cvars[kMaxCvars];
static int32_t ms_count;

void cvar_create(cvar_t* ptr, const char* name, const char* value)
{
    ASSERT(ptr);
    ASSERT(name);
    ASSERT(value);

    cvar_t* extant = cvar_find(name);
    if (extant)
    {
        ASSERT(ptr == extant);
    }
    else
    {
        ASSERT(ms_count < kMaxCvars);
        int32_t i = ms_count++;
        ms_hashes[i] = HashStr(name);
        ms_cvars[i] = ptr;
        strcpy_s(ptr->name, NELEM(ptr->name), name);
        cvar_set_str(ptr, value);
    }
}

cvar_t* cvar_find(const char* name)
{
    ASSERT(name);
    int32_t i = HashFind(ms_hashes, ms_count, HashStr(name));
    return (i == -1) ? 0 : ms_cvars[i];
}

void cvar_set_str(cvar_t* ptr, const char* value)
{
    ASSERT(ptr);
    ASSERT(value);
    strcpy_s(ptr->value, NELEM(ptr->value), value);
    ptr->asFloat = (float)atof(value);
}

void cvar_set_float(cvar_t* ptr, float value)
{
    ASSERT(ptr);
    sprintf_s(ptr->value, NELEM(ptr->value), "%f", value);
    ptr->asFloat = value;
}
