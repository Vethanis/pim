#include "common/cvar.h"

#include "allocator/allocator.h"
#include "common/hashstring.h"
#include "common/stringutil.h"
#include "containers/dict.h"
#include <stdlib.h> // atof

static dict_t ms_dict;

static void EnsureDict(void)
{
    if (!ms_dict.valueSize)
    {
        dict_new(&ms_dict, sizeof(cvar_t*), EAlloc_Perm);
    }
}

void cvar_reg(cvar_t* ptr)
{
    EnsureDict();

    ASSERT(ptr);
    ASSERT(ptr->name);
    ASSERT(ptr->description);
    ptr->asFloat = (float)atof(ptr->value);

    bool added = dict_add(&ms_dict, ptr->name, &ptr);
    ASSERT(added);
}

void cvar_create(cvar_t* ptr, const char* name, const char* value, const char* desc)
{
    ASSERT(ptr);
    ASSERT(name);
    ASSERT(value);
    ASSERT(desc);
    ptr->name = name;
    ptr->description = desc;
    cvar_set_str(ptr, value);
    cvar_reg(ptr);
}

cvar_t* cvar_find(const char* name)
{
    EnsureDict();

    ASSERT(name);
    const i32 i = dict_find(&ms_dict, name);
    if (i != -1)
    {
        cvar_t** cvars = ms_dict.values;
        return cvars[i];
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
