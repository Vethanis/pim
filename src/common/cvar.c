#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/hashstring.h"
#include "common/stringutil.h"
#include <stdlib.h>

#define kMaxCvars 256

static u32 ms_hashes[kMaxCvars];
static cvar_t* ms_cvars[kMaxCvars];
static i32 ms_count;

void cvar_reg(cvar_t* ptr)
{
    ASSERT(ptr);
    ASSERT(ptr->name);
    ASSERT(ptr->value);
    ASSERT(ptr->description);
    ptr->asFloat = (float)atof(ptr->value);

    u32 hash = HashStr(ptr->name);
    i32 i = HashFind(ms_hashes, ms_count, hash);
    if (i != -1)
    {
        ASSERT(ptr == ms_cvars[i]);
        ASSERT(StrCmp(ms_cvars[i]->name, 32, ptr->name) == 0);
    }
    else
    {
        ASSERT(ms_count < kMaxCvars);
        i = ms_count++;
        ms_hashes[i] = hash;
        ms_cvars[i] = ptr;
    }
}

void cvar_create(cvar_t* ptr, const char* name, const char* value, const char* desc)
{
    ASSERT(ptr);
    ASSERT(name);
    ASSERT(value);
    ASSERT(desc);
    ptr->name = name;
    ptr->description = desc;

    u32 hash = HashStr(name);
    i32 i = HashFind(ms_hashes, ms_count, hash);
    if (i != -1)
    {
        ASSERT(ptr == ms_cvars[i]);
        ASSERT(StrCmp(ms_cvars[i]->name, 32, ptr->name) == 0);
    }
    else
    {
        ASSERT(ms_count < kMaxCvars);
        i = ms_count++;
        ms_hashes[i] = hash;
        ms_cvars[i] = ptr;
        cvar_set_str(ptr, value);
    }
}

cvar_t* cvar_find(const char* name)
{
    ASSERT(name);
    i32 i = HashFind(ms_hashes, ms_count, HashStr(name));
    if (i != -1)
    {
        ASSERT(StrCmp(ms_cvars[i]->name, 32, name) == 0);
        return ms_cvars[i];
    }
    return NULL;
}

void cvar_set_str(cvar_t* ptr, const char* value)
{
    ASSERT(ptr);
    ASSERT(value);
    StrCpy(ARGS(ptr->value), value);
    ptr->asFloat = (float)atof(value);
}

void cvar_set_float(cvar_t* ptr, float value)
{
    ASSERT(ptr);
    SPrintf(ARGS(ptr->value), "%f", value);
    ptr->asFloat = value;
}
