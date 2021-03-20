#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef enum
{
    cvart_text = 0,
    cvart_float,
    cvart_int,
    cvart_bool,
    cvart_vector,   // unit direction
    cvart_point,    // position
    cvart_color,    // LDR color
} cvar_type;

typedef enum
{
    cvarf_dirty = 1 << 0,   // modified in the last frame
    cvarf_save = 1 << 1,    // save value to config file

} cvar_flags;

typedef struct ConVar_s
{
    cvar_type type;
    u32 flags;
    const char* name;
    char value[64];
    const char* desc;
    union
    {
        i32 minInt;
        float minFloat;
    };
    union
    {
        i32 maxInt;
        float maxFloat;
    };
    union
    {
        float4 asVector;
        float asFloat;
        i32 asInt;
        bool asBool;
    };
} ConVar_t;

// registers your cvar to the cvar system
void cvar_reg(ConVar_t* ptr);

// attempts to find a cvar with matching name
ConVar_t* cvar_find(const char* name);

// attempts to auto-complete namePart to a cvar name
const char* cvar_complete(const char* namePart);

// updates string and float value, sets dirty flag
void cvar_set_str(ConVar_t* ptr, const char* value);
void cvar_set_float(ConVar_t* ptr, float value);
void cvar_set_int(ConVar_t* ptr, i32 value);
void cvar_set_vec(ConVar_t* ptr, float4 value);
void cvar_set_bool(ConVar_t* ptr, bool value);

pim_inline bool cvar_is_vec(const ConVar_t* ptr)
{
    switch (ptr->type)
    {
    default:
        return false;
    case cvart_vector:
    case cvart_point:
    case cvart_color:
        return true;
    }
}

pim_inline const char* cvar_get_str(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_text);
    return ptr->value;
}
pim_inline float cvar_get_float(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_float);
    return ptr->asFloat;
}
pim_inline i32 cvar_get_int(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_int);
    return ptr->asInt;
}
pim_inline float4 cvar_get_vec(const ConVar_t* ptr)
{
    ASSERT(cvar_is_vec(ptr));
    return ptr->asVector;
}
pim_inline bool cvar_get_bool(const ConVar_t* ptr)
{
    ASSERT(ptr->type == cvart_bool);
    return ptr->asBool;
}

pim_inline void cvar_toggle(ConVar_t* ptr) { cvar_set_bool(ptr, !cvar_get_bool(ptr)); }

// checks and clears dirty flag
bool cvar_check_dirty(ConVar_t* ptr);

// displays the cvar gui
void cvar_gui(bool* pEnabled);


PIM_C_END
